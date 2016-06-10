#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <cinttypes>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "silo.h"

using namespace std;
using namespace silo;

static const char *meta = "meta";

block::block(const char *dir)
	: basedir(dir)
{
	if (basedir.back() != '/') basedir.push_back('/');
}

static string to_string(const md5val &hash)
{
	string ret;
	ret.reserve(32);

	char buf[8];

	for (int i = 0; i < 16; i++)
	{
		sprintf(buf, "%02x", hash[i]);
		ret += buf;
	}

	return ret;
}

chunk block::getchunk(const md5val &hash)
{
	string name = basedir + to_string(hash);

	int fd = open(name.c_str(), O_RDONLY);
	if (fd == -1)
	{
		fprintf(stderr, "chunk %s not found\n", name.c_str());
		exit(1);
	}

	struct stat st;
	fstat(fd, &st);

	chunk ret;
	chunk_file_header header;

	if (st.st_size < sizeof(header))
	{
		fprintf(stderr, "chunk %s has corrupted\n", name.c_str());
		exit(1);
	}

	if (read(fd, &header, sizeof(header)) < sizeof(header))
	{
		fprintf(stderr, "IO error\n");
		exit(1);
	}

	ret.type = header.type;

	ret.blob.resize(st.st_size - sizeof(header));
	read(fd, &ret.blob[0], st.st_size - sizeof(header));

	return ret;
}

void block::addchunk(const chunk &chk, uint64_t initref)
{
	string name = basedir + to_string(chk.hash);

	int fd = open(name.c_str(), O_RDWR | O_CREAT, 0775);
	if (fd == -1)
	{
		fprintf(stderr, "IO error\n");
		exit(1);
	}

	if (initref <= 0)
	{
		fprintf(stderr, "initref is %" PRIu64 "\n", initref);
		exit(1);
	}

	struct stat st;
	fstat(fd, &st);
	if (st.st_size == 0)
	{
		chunk_file_header header{initref, chk.type};
		write(fd, &header, sizeof(header));
		write(fd, &chk.blob[0], chk.blob.size());
	}
	else
	{
		chunk_file_header header;
		read(fd, &header, sizeof(header));
		header.refcount += initref;

		lseek(fd, 0, SEEK_SET);
		write(fd, &header, sizeof(header));
	}

	close(fd);
}

void block::releasechunk(const md5val &hash)
{
	string name = basedir + to_string(hash);

	int fd = open(name.c_str(), O_RDWR, 0775);
	if (fd == -1) return;

	chunk_file_header header;
	read(fd, &header, sizeof(header));

	if (header.refcount <= 1)
	{
		close(fd);
		unlink(name.c_str());
	}
	else
	{
		header.refcount--;
		lseek(fd, 0, SEEK_SET);
		write(fd, &header, sizeof(header));
		close(fd);
	}
}
