/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <vector>
#include <string>
#include <atomic>
using namespace std;

struct filedata
{
	vector<char> blob;
	bool opened = false;
	bool append = false;
	bool rd = false;
	bool wr = false;
};

map<string, filedata> files;

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
		auto &&fdt = itr->second;
		stbuf->st_mode = S_IFREG | 0755;
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

	if ((fi->flags & 3) != O_RDONLY) return -EACCES;

	auto &&fdt = itr->second;

	if (fdt.opened)
	{
		return -EACCES;
	}

	switch (fi->flags & 3)
	{
		case O_RDONLY: fdt.rd = true; break;
		case O_WRONLY: fdt.wr = true; break;
		case O_RDWR: fdt.rd = fdt.wr = true; break;
	}

	fdt.opened = true;

	return 0;
}

static int silo_create(const char *path, mode_t, struct fuse_file_info *)
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

	filedata fd;
	fd.opened = true;
	fd.rd = fd.wr = true;
	files.emplace(path, move(fd));

	return 0;
}

static int silo_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	auto itr = files.find(path);
	if (itr == files.end())
		return -ENOENT;

	auto &&fdt = itr->second;
	if (fdt.rd == false) return -EBADF;

	len = fdt.blob.size();
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, &fdt.blob[offset], size);
	} else
		size = 0;

	return size;
}

static int silo_write(const char *path, const char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	auto itr = files.find(path);
	if (itr == files.end())
		return -ENOENT;

	auto &&fdt = itr->second;
	if (fdt.wr == false) return -EBADF;

	len = fdt.blob.size();
	if (offset + size > len)
	{
		fdt.blob.resize(offset + size);
	}

	memcpy(&fdt.blob[offset], buf, size);

	return size;
}

static int silo_release(const char *path, struct fuse_file_info *fi)
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		return -ENOENT;
	}

	itr->second.opened = false;

	return 0;
}

static int silo_unlink(const char *path)
{
	auto itr = files.find(path);
	if (itr == files.end())
	{
		return -ENOENT;
	}
	if (itr->second.opened)
	{
		return -EBUSY;
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
	if (itr->second.opened)
	{
		return -EBUSY;
	}

	itr->second.blob.resize(size);

	return 0;
}

static struct fuse_operations silo_oper;

int main(int argc, char *argv[])
{
	filedata tmp;
	const char *hello = "hello\n";
	tmp.blob.insert(tmp.blob.end(), hello, hello + strlen(hello));
	files["/hello"] = move(tmp);
	silo_oper.getattr = silo_getattr;
	silo_oper.readdir = silo_readdir;
	silo_oper.open = silo_open;
	silo_oper.create = silo_create;
	silo_oper.read = silo_read;
	silo_oper.write = silo_write;
	silo_oper.unlink = silo_unlink;
	silo_oper.release = silo_release;
	silo_oper.truncate = silo_truncate;
	return fuse_main(argc, argv, &silo_oper, NULL);
}
