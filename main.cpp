#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <stdarg.h>
#include "silo.h"
using namespace std;
using namespace silo;

struct filedata
{
	vector<char> blob;
	time_t atime{ 0 };
	time_t mtime{ 0 };
	time_t ctime{ 0 };
	int opencount;
	bool modified;
};

struct filedesc
{
	shared_ptr<filedata> pfd;
	bool rd{ false };
	bool wr{ false };
	bool ap{ false };
};

char basedir[200];
map<string, shared_ptr<filedata>> files;
map<string, int64_t> dircount;
unique_ptr<silofs> psil;
#define sil (*psil)

static bool isdir(const char *path)
{
	struct stat st;
	if (stat(path, &st) == -1) return false;
	return S_ISDIR(st.st_mode);
}

static int silo_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	string volpath = sil.getvolumepath(path);

	if (isdir(volpath.c_str()))
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else
	{
		auto itr = files.find(path);
		if (itr == files.end()) {
			file_header header;
			if (sil.header(path, header) == false) return -ENOENT;
			stbuf->st_atime = header.atime;
			stbuf->st_mtime = header.mtime;
			stbuf->st_ctime = header.ctime;
			stbuf->st_size = header.size;
		}
		else
		{
			auto &&fdt = *itr->second;
			stbuf->st_atime = fdt.atime;
			stbuf->st_mtime = fdt.mtime;
			stbuf->st_ctime = fdt.ctime;
			stbuf->st_size = fdt.blob.size();
		}
		stbuf->st_mode = S_IFREG | 0755;
		stbuf->st_nlink = 1;
	}

	return 0;
}

static int silo_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	string volpath = sil.getvolumepath(path);

	if (isdir(volpath.c_str()) == false) return -ENOENT;

	DIR *d = opendir(volpath.c_str());
	struct dirent *dir;


	while ((dir = readdir(d)) != nullptr)
	{
		if (files.find(dir->d_name) == files.end())
		{
			filler(buf, dir->d_name, nullptr, 0);
		}
	}

	string search = path;
	if (search.back() != '/') search.push_back('/');

	sil.foreachpendingname([&](const string &name){
		if (strncmp(name.c_str(), search.c_str(), search.size()) == 0 && files.find(name) == files.end())
		{
			if (strchr(name.c_str() + search.size(), '/')) return true;
			filler(buf, name.c_str() + search.size(), nullptr, 0);
		}
		return true;
	});

	closedir(d);

	for (auto &&elem : files)
	{
		if (strncmp(elem.first.c_str(), search.c_str(), search.size()) == 0)
		{
			if (strchr(elem.first.c_str() + search.size(), '/')) continue;
			filler(buf, elem.first.c_str() + search.size(), NULL, 0);
		}
	}

	return 0;
}

static int silo_opendir(const char *path, struct fuse_file_info* fi)
{
	(void) fi;

	string volpath = sil.getvolumepath(path);
	if (isdir(volpath.c_str()) == false) return -ENOENT;

	dircount[path]++;

	return 0;
}

static int silo_releasedir(const char* path, struct fuse_file_info *fi)
{
	(void) fi;

	if (--dircount[path] <= 0) dircount.erase(path);

	return 0;
}

static int silo_mkdir(const char* path, mode_t mode)
{
	(void) mode;

	string volpath = sil.getvolumepath(path);
	if (mkdir(volpath.c_str(), 0755) == -1)
	{
		return -errno;
	}

	return 0;
}

static int silo_rmdir(const char* path)
{
	string volpath = sil.getvolumepath(path);

	if (isdir(volpath.c_str()) == false) return -ENOENT;
	auto itr = dircount.find(path);
	if (itr != dircount.end() && itr->second >= 1)
	{
		return -EBUSY;
	}

	size_t pathlen = strlen(path);
	bool wrong = false;
	sil.foreachpendingname([&](const string &name){
		if (strncmp(&name[0], path, pathlen) == 0)
		{
			wrong = true;
			return false;
		}
		return true;
	});

	if (wrong) return -ENOTEMPTY;

	for (auto &&elem : files)
	{
		if (strncmp(elem.first.c_str(), path, pathlen) == 0) return -ENOTEMPTY;
	}

	if (rmdir(volpath.c_str()) == -1) return -errno;

	return 0;
}

static int silo_open(const char *path, struct fuse_file_info *fi)
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		file_header header;
		if (sil.header(path, header) == false)
		{
			return -ENOENT;
		}

		vector<char> blob;
		sil.read(path, blob);

		itr = files.emplace(path, make_shared<filedata>()).first;
		itr->second->blob = move(blob);
		itr->second->mtime = header.mtime;
		itr->second->ctime = header.ctime;
	}

	itr->second->atime = time(nullptr);
	auto pd = new filedesc();
	pd->pfd = itr->second;

	switch (fi->flags & 3)
	{
		case O_RDONLY: pd->rd = true; break;
		case O_WRONLY: pd->wr = true; break;
		case O_RDWR: pd->rd = pd->wr = true; break;
	}

	fi->fh = reinterpret_cast<uint64_t>(pd);

	itr->second->opencount++;

	return 0;
}

static int silo_create(const char *path, mode_t, struct fuse_file_info *fi)
{
	auto itr = files.find(path);
	if (itr != files.end())
	{
		return -EEXIST;
	}

	file_header header;
	if (sil.header(path, header)) return -EEXIST;

	for (size_t i = strlen(path) - 1; i != -1; i--)
	{
		if (path[i] == '/')
		{
			if (isdir(sil.getvolumepath(string(path, path + i + 1).c_str()).c_str()) == false)
				return -ENOENT;

			break;
		}
	}

	auto pfd = make_shared<filedata>();
	auto &&fd = *pfd;
	fd.opencount = 1;
	fd.modified = true;
	fd.atime = fd.mtime = fd.ctime = time(nullptr);
	files.emplace(path, pfd);

	auto pd = new filedesc();
	pd->pfd = pfd;
	pd->rd = pd->wr = true;
	fi->fh = reinterpret_cast<uint64_t>(pd);

	return 0;
}

static int silo_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;

	auto &&desc = *reinterpret_cast<filedesc *>(fi->fh);
	if (desc.rd == false) return -EBADF;

	auto &&blob = desc.pfd->blob;

	len = blob.size();
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, &blob[offset], size);
	} else
		size = 0;

	return size;
}

static int silo_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;

	auto &&desc = *reinterpret_cast<filedesc *>(fi->fh);
	if (desc.wr == false) return -EBADF;

	auto &&fdt = *desc.pfd;
	auto &&blob = fdt.blob;

	len = blob.size();
	if (offset + size > len)
	{
		blob.resize(offset + size);
	}

	memcpy(&blob[offset], buf, size);

	fdt.ctime = fdt.mtime = time(nullptr);
	fdt.modified = true;

	return size;
}

static int silo_release(const char *path, struct fuse_file_info *fi)
{
	auto &&desc = *reinterpret_cast<filedesc *>(fi->fh);

	if (--desc.pfd->opencount == 0)
	{
		file_header header;
		header.atime = desc.pfd->atime;
		header.mtime = desc.pfd->mtime;
		header.ctime = desc.pfd->ctime;

		if (desc.pfd->modified)
		{
			sil.write(path, desc.pfd->blob.empty() ? nullptr : &desc.pfd->blob[0],
						desc.pfd->blob.size(), header);
		}
		else
		{
			sil.writeheader(path, header);
		}

		auto itr = files.find(path);
		if (itr != files.end() && itr->second == desc.pfd)
		{
			files.erase(itr);
		}
	}

	delete &desc;

	return 0;
}

static int silo_unlink(const char *path)
{
	auto itr = files.find(path);
	if (itr != files.end())
	{
		files.erase(itr);
		sil.remove(path);
		return 0;
	}
	
	if (sil.remove(path)) return 0;
	else return -ENOENT;
}

static int silo_truncate(const char *path, off_t size)
{
	auto itr = files.find(path);
	if (itr != files.end())
	{
		auto &&fdt = *itr->second;
		fdt.blob.resize(size);
		fdt.atime = fdt.mtime = fdt.ctime = time(nullptr);
		return 0;
	}

	file_header h;
	if (sil.header(path, h) == false) return -ENOENT;
	h.atime = h.mtime = h.ctime = time(nullptr);
	sil.remove(path);
	sil.write(path, nullptr, 0, h);

	return 0;
}

static int silo_utimens(const char *path, const struct timespec ts[2])
{
	auto itr = files.find(path);
	if (itr != files.end())
	{
		auto &&fdt = *itr->second;

		fdt.atime = ts[0].tv_sec;
		fdt.mtime = ts[1].tv_sec;

		return 0;
	}

	string volpath = sil.getvolumepath(path);
	if (isdir(volpath.c_str()))
	{
		return -ENOSYS;
	}

	file_header h;
	if (sil.header(path, h) == false) return -ENOENT;
	h.atime = ts[0].tv_sec;
	h.mtime = ts[1].tv_sec;
	sil.writeheader(path, h);

	return 0;
}

static struct fuse_operations silo_oper;

int main(int argc, char *argv[])
{
	FILE *fp = fopen("fusebase", "r");
	if (fp == nullptr)
	{
		printf("warning: no setting file. specify a base directory\n");
		fgets(basedir, 200, stdin);
	}
	else
	{
		fgets(basedir, 200, fp);
		fclose(fp);
	}

	size_t len = strlen(basedir);
	if (basedir[len-1] == '\n') basedir[len-1] = '\0';

	psil = make_unique<silofs>(basedir);
	silo_oper.getattr = silo_getattr;
	silo_oper.readdir = silo_readdir;
	silo_oper.open = silo_open;
	silo_oper.create = silo_create;
	silo_oper.read = silo_read;
	silo_oper.write = silo_write;
	silo_oper.unlink = silo_unlink;
	silo_oper.release = silo_release;
	silo_oper.truncate = silo_truncate;
	silo_oper.utimens = silo_utimens;
	silo_oper.opendir = silo_opendir;
	silo_oper.releasedir = silo_releasedir;
	silo_oper.mkdir = silo_mkdir;
	silo_oper.rmdir = silo_rmdir;

//	freopen("errorlog", "w", stderr);

	return fuse_main(argc, argv, &silo_oper, NULL);
}
