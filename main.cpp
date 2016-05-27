#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
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
using namespace std;

struct filedata
{
	vector<char> blob;
	time_t atime{ 0 };
	time_t mtime{ 0 };
	time_t ctime{ 0 };
};

struct filedesc
{
	shared_ptr<filedata> pfd;
	bool rd{ false };
	bool wr{ false };
	bool ap{ false };
};

map<string, shared_ptr<filedata>> files;

static int silo_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		auto itr = files.find(path);
		if (itr == files.end()) {
			return -ENOENT;
		}
		auto &&fdt = *itr->second;
		stbuf->st_mode = S_IFREG | 0755;
		stbuf->st_atime = fdt.atime;
		stbuf->st_mtime = fdt.mtime;
		stbuf->st_ctime = fdt.ctime;
		stbuf->st_nlink = 1;
		stbuf->st_size = fdt.blob.size();
	}

	return 0;
}

static int silo_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	for (auto &&elem : files)
	{
		filler(buf, elem.first.c_str() + 1, NULL, 0);
	}

	return 0;
}

static int silo_open(const char *path, struct fuse_file_info *fi)
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		return -ENOENT;
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

	return 0;
}

static int silo_create(const char *path, mode_t, struct fuse_file_info *fi)
{
	auto itr = files.find(path);
	if (itr != files.end())
	{
		return -EEXIST;
	}
	for (int i = 1; path[i]; i++)
	{
		if (path[i] == '/')
		{
			return -EACCES;
		}
	}

	auto pfd = make_shared<filedata>();
	auto &&fd = *pfd;
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

	return size;
}

static int silo_release(const char *path, struct fuse_file_info *fi)
{
	delete reinterpret_cast<filedesc *>(fi->fh);

	return 0;
}

static int silo_unlink(const char *path)
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		return -ENOENT;
	}

	files.erase(itr);

	return 0;
}

static int silo_truncate(const char *path, off_t size)
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		return -ENOENT;
	}

	auto &&fdt = *itr->second;

	fdt.blob.resize(size);

	fdt.atime = fdt.mtime = fdt.ctime = time(nullptr);

	return 0;
}

static int silo_utimens(const char *path, const struct timespec ts[2])
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		return -ENOENT;
	}

	auto &&fdt = *itr->second;

	fdt.atime = ts[0].tv_sec;
	fdt.mtime = ts[1].tv_sec;

	return 0;
}

static struct fuse_operations silo_oper;

int main(int argc, char *argv[])
{
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
	return fuse_main(argc, argv, &silo_oper, NULL);
}
