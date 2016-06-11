#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <algorithm>
#include <dirent.h>
#include <zlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "lib/libsilo.h"
#include "silo.h"
using namespace std;
using namespace silo;

void segment_buffer::deserialize(const char *file)
{
	int fd = open(file, O_RDONLY);
	if (fd == -1) return;

	for (;;)
	{
		md5val hash;
		chunk_file_header header;
		if (read(fd, &hash[0], hash.size()) != hash.size() ||
			read(fd, &header, sizeof(header)) != sizeof(header))
			break;

		chunk chk;
		chk.type = header.type;
		chk.hash = hash;
		chk.rawsize = header.rawsize;
		size += chk.rawsize;

		uint32_t blobsize;
		if (read(fd, &blobsize, sizeof(blobsize)) != sizeof(blobsize)) break;

		chk.blob.resize(blobsize);
		if (read(fd, &chk.blob[0], blobsize) != blobsize) break;

		// to avoid packed field error
		auto refcount = header.refcount;
		chunks.emplace_hint(chunks.end(), hash, make_pair(refcount, move(chk)));
	}

	close(fd);
}

void segment_buffer::serialize(const char *file)
{
	int fd = open(file, O_WRONLY | O_CREAT, 0755);
	if (fd == -1) return;

	for (auto &&elem : chunks)
	{
		auto &&hash = elem.first;
		auto refcount = elem.second.first;
		auto &&chk = elem.second.second;

		chunk_file_header header;
		header.refcount = refcount;
		header.type = chk.type;
		header.rawsize = chk.rawsize;

		write(fd, &hash[0], hash.size());
		write(fd, &header, sizeof(header));

		auto blobsize = static_cast<uint32_t>(chk.blob.size());
		write(fd, &blobsize, sizeof(blobsize));
		write(fd, &chk.blob[0], blobsize);
	}

	close(fd);
}

void block_buffer::deserialize(const char *file)
{
	int fd = open(file, O_RDONLY);
	if (fd == -1) return;

	uint32_t segcount;
	if (read(fd, &segcount, sizeof(segcount)) != sizeof(segcount))
	{
		fprintf(stderr, "blkbuf deserialize fail\n");
		exit(1);
	}

	while (segcount--)
	{
		md5val hash;
		if (read(fd, &hash[0], hash.size()) != hash.size())
		{
			fprintf(stderr, "blkbuf deserialize fail\n");
			exit(1);
		}
	}

	for (;;)
	{
		md5val hash;
		chunk_file_header header;
		if (read(fd, &hash[0], hash.size()) != hash.size() ||
			read(fd, &header, sizeof(header)) != sizeof(header))
			break;

		chunk chk;
		chk.type = header.type;
		chk.hash = hash;
		chk.rawsize = header.rawsize;

		uint32_t blobsize;
		if (read(fd, &blobsize, sizeof(blobsize)) != sizeof(blobsize)) break;

		chk.blob.resize(blobsize);
		if (read(fd, &chk.blob[0], blobsize) != blobsize) break;

		// to avoid packed field error
		auto refcount = header.refcount;
		chunks.emplace_hint(chunks.end(), hash, make_pair(refcount, move(chk)));
	}

	close(fd);
}

void block_buffer::serialize(const char *file)
{
	int fd = open(file, O_WRONLY | O_CREAT, 0755);
	if (fd == -1) return;

	uint32_t segcount = segments.size();
	write(fd, &segcount, sizeof(segcount));

	for (auto &&hash : segments)
	{
		write(fd, &hash[0], hash.size());
	}

	for (auto &&elem : chunks)
	{
		auto &&hash = elem.first;
		auto refcount = elem.second.first;
		auto &&chk = elem.second.second;

		chunk_file_header header;
		header.refcount = refcount;
		header.type = chk.type;
		header.rawsize = chk.rawsize;

		write(fd, &hash[0], hash.size());
		write(fd, &header, sizeof(header));

		auto blobsize = static_cast<uint32_t>(chk.blob.size());
		write(fd, &blobsize, sizeof(blobsize));
		write(fd, &chk.blob[0], blobsize);
	}

	close(fd);
}

silofs::silofs(const char *basedir) : base(basedir)
{
	if (base.back() != '/') base.push_back('/');

	mkdir((base + volume_dir).c_str(), 0755);

	segbuf.deserialize((base + segbuf_file).c_str());
	blkbuf.deserialize((base + blkbuf_file).c_str());

	shtbl.load((base + shtable_file).c_str());

	string blkdir = base;
	size_t p = blkdir.size();
	blkdir.resize(blkdir.size() + 15);
	for (lastblock = 0;; lastblock++)
	{
		sprintf(&blkdir[p], "%d", lastblock);
		struct stat st;
		if (stat(blkdir.c_str(), &st) == -1) break;
		if (!S_ISDIR(st.st_mode))
		{
			fprintf(stderr, "%s is not a directory\n", blkdir.c_str());
			exit(1);
		}

		blocks.emplace_back(blkdir.c_str());
	}

	int pfd = open((base + pending_name).c_str(), O_RDONLY);
	if (pfd == -1) return;

	for (;;)
	{
		uint32_t namelen;
		if (::read(pfd, &namelen, sizeof(namelen)) != sizeof(namelen)) break;

		string name(namelen, char{});
		if (::read(pfd, &name[0], namelen) != namelen)
		{
			fprintf(stderr, "pending file error\n");
			exit(1);
		}

		auto &&pending = pendings[name];
		if (::read(pfd, &pending.header, sizeof(pending.header)) != sizeof(pending.header) ||
			::read(pfd, &pending.lastseg, sizeof(pending.lastseg)) != sizeof(pending.lastseg) ||
			::read(pfd, &pending.lastblk, sizeof(pending.lastblk)) != sizeof(pending.lastblk) ||
			::read(pfd, &pending.left, sizeof(pending.left)) != sizeof(pending.left))
		{
			fprintf(stderr, "pending file error\n");
			exit(1);
		}

		uint32_t chkcnt;
		if (::read(pfd, &chkcnt, sizeof(chkcnt)) != sizeof(chkcnt))
		{
			fprintf(stderr, "pending file error\n");
			exit(1);
		}

		while (chkcnt--)
		{
			chunk_info info;
			if (::read(pfd, &info, sizeof(info)) != sizeof(info))
			{
				fprintf(stderr, "pending file error\n");
				exit(1);
			}

			pending.chunks.push_back(info);
		}
	}

	close(pfd);
}

silofs::~silofs()
{
	segbuf.serialize((base + segbuf_file).c_str());
	blkbuf.serialize((base + blkbuf_file).c_str());

	shtbl.save((base + shtable_file).c_str());
	
	int pfd = open((base + pending_name).c_str(), O_WRONLY | O_CREAT, 0755);
	if (pfd == -1) return;

	for (auto &&elem : pendings)
	{
		auto &&name = elem.first;
		auto &&pending = elem.second;
		uint32_t namelen = name.size();
		if (::write(pfd, &namelen, sizeof(namelen)) != sizeof(namelen)) break;
		if (::write(pfd, &name[0], namelen) != namelen) break;

		if (::write(pfd, &pending.header, sizeof(pending.header)) != sizeof(pending.header) ||
			::write(pfd, &pending.lastseg, sizeof(pending.lastseg)) != sizeof(pending.lastseg) ||
			::write(pfd, &pending.lastblk, sizeof(pending.lastblk)) != sizeof(pending.lastblk) ||
			::write(pfd, &pending.left, sizeof(pending.left)) != sizeof(pending.left))
			break;

		uint32_t chkcnt = pending.chunks.size();
		if (::write(pfd, &chkcnt, sizeof(chkcnt)) != sizeof(chkcnt)) break;

		for (auto &&info : pending.chunks)
		{
			if (::write(pfd, &info, sizeof(info)) != sizeof(info)) break;
		}
	}

	close(pfd);
}

bool silofs::read(const char *file, vector<char> &ret)
{
	ret.clear();

	auto pit = pendings.find(file);
	if (pit != pendings.end())
	{
		for (auto &&info : pit->second.chunks)
		{
			auto blob = readchunk(info);
			ret.insert(ret.end(), blob.begin(), blob.end());
		}

		return true;
	}
	else
	{
		string meta = base + volume_dir + file;
		int fd = open(meta.c_str(), O_RDONLY);
		if (fd == -1)
		{
			return false;
		}

		file_header header;
		if (::read(fd, &header, sizeof(header)) != sizeof(header))
		{
			fprintf(stderr, "%s reading error\n", file);
			exit(1);
		}

		for (off_t i = 0, e = header.chunks; i < e; i++)
		{
			chunk_info info;
			if (::read(fd, &info, sizeof(info)) != sizeof(info))
			{
				fprintf(stderr, "%s reading error\n", file);
				exit(1);
			}

			auto blob = readchunk(info);
			ret.insert(ret.end(), blob.begin(), blob.end());
		}

		return true;
	}
}


bool silofs::header(const char *file, file_header &header)
{
	auto pit = pendings.find(file);
	if (pit != pendings.end())
	{
		header = pit->second.header;
		fprintf(stderr, "header function size %jd\n", header.size);
		return true;
	}
	else
	{
		string meta = base + volume_dir + file;
		int fd = open(meta.c_str(), O_RDONLY);
		if (fd == -1) return false;

		file_header ret;
		if (::read(fd, &header, sizeof(header)) != sizeof(header))
		{
			fprintf(stderr, "%s reading error\n", file);
			exit(1);
		}

		fprintf(stderr, "header function size %jd\n", header.size);

		close(fd);
		return true;
	}
}

void silofs::write(const char *file, const void *dat, size_t size, file_header header)
{
	remove(file);

	fprintf(stderr, "trying to write %s of size %jd\n", file, size);

	vector<size_t> chunked;
	do_chunking(dat, size, chunked);

	fprintf(stderr, "chunk completed with number %jd\n", chunked.size());

	pending_file &pending = pendings[file];

	header.size = size;
	header.chunks = chunked.size();

	pending.header = header;
	pending.chunks.resize(chunked.size(), {chunk_info::pending, {}});
	pending.left = chunked.size();

	size_t last = 0;
	const char *dat_begin = static_cast<const char *>(dat);
	for (size_t i = 0; i < chunked.size(); i++)
	{
		size_t next = chunked[i];
		fprintf(stderr, "current chunk: %jd to %jd (%jd bytes)\n", last, next, next - last);
		auto chk = chunk::frombuffer(dat_begin + last, next - last);
		last = next;

		pending.chunks[i] = {chunk_info::segbuf, chk.hash};

		auto inserted = segbuf.chunks.emplace(chk.hash, make_pair(1, move(chk)));
		if (inserted.second == false)
		{
			// inserted.first: itr, itr->second : pair<count, chk>
			inserted.first->second.first++;
			continue;
		}

		segbuf.size += next - last;
		if (segbuf.size < SEGMENT_SIZE) continue;

		md5val repid = segbuf.chunks.begin()->first;
		int blkidx;
		if (shtbl.find(blkidx, repid))
		{
			segtoblkidx(blkidx);
		}
		else
		{
			segtoblkbuf();
		}

		if (blkbuf.segments.size() >= SEGMENTS_PER_BLOCK)
		{
			flushblkbuf();
		}
	}

	flushpending();
}

void silofs::writeheader(const char *file, file_header header)
{
	fprintf(stderr, "writeheader size %jd\n", header.size);
	auto pit = pendings.find(file);
	if (pit != pendings.end())
	{
		fprintf(stderr, "writeheader size %jd\n", header.size);
		pit->second.header.atime = header.atime;
		pit->second.header.mtime = header.mtime;
		pit->second.header.ctime = header.ctime;
	}
	else
	{
		string meta = base + volume_dir + file;
		int fd = open(meta.c_str(), O_RDWR);
		if (fd == -1)
		{
			fprintf(stderr, "%s not found\n", file);
			exit(1);
		}

		file_header newheader;

		if (::read(fd, &newheader, sizeof(newheader)) != sizeof(newheader))
		{
			fprintf(stderr, "%s read error\n", file);
			exit(1);
		}

		fprintf(stderr, "writeheader before %jd\n", newheader.size);

		newheader.atime = header.atime;
		newheader.mtime = header.mtime;
		newheader.ctime = header.ctime;

		lseek(fd, 0, SEEK_SET);
		if (::write(fd, &newheader, sizeof(newheader)) != sizeof(newheader))
		{
			fprintf(stderr, "%s write error\n", file);
			exit(1);
		}

		close(fd);
	}
}

bool silofs::remove(const char *file)
{
	auto pit = pendings.find(file);
	if (pit != pendings.end())
	{
		for (auto &&info : pit->second.chunks)
		{
			releasechunk(info);
		}

		pendings.erase(pit);

		return true;
	}
	else
	{
		string meta = base + volume_dir + file;
		int fd = open(meta.c_str(), O_RDONLY);
		if (fd == -1)
		{
			return false;
		}

		file_header header;
		if (::read(fd, &header, sizeof(header)) != sizeof(header))
		{
			fprintf(stderr, "%s reading error 1 %jd\n", file, sizeof(header));
			exit(1);
		}

		for (off_t i = 0, e = header.chunks; i < e; i++)
		{
			chunk_info info;
			if (::read(fd, &info, sizeof(info)) != sizeof(info))
			{
				fprintf(stderr, "%s reading error 2 %jd\n", file, sizeof(info));
				exit(1);
			}
			releasechunk(info);
		}

		close(fd);
		unlink(meta.c_str());
		return true;
	}
}

vector<char> silofs::readchunk(chunk_info info)
{
	if (info.blk_idx < 0)
	{
		auto &&chunks = info.blk_idx == chunk_info::segbuf ?
			segbuf.chunks : blkbuf.chunks;
		auto cit = chunks.find(info.hash);
		if (cit == chunks.end())
		{
			fprintf(stderr, "blockbuffer not found\n");
			exit(1);
		}

		return cit->second.second.unzip();
	}
	else
	{
		if (info.blk_idx >= blocks.size())
		{
			fprintf(stderr, "block %d not found\n", info.blk_idx);
			exit(1);
		}
		return blocks[info.blk_idx].getchunk(info.hash).unzip();
	}
}

void silofs::releasechunk(chunk_info info)
{
	switch (info.blk_idx)
	{
		case chunk_info::segbuf:
			--segbuf.chunks[info.hash].first;
			break;
		case chunk_info::blkbuf:
			--blkbuf.chunks[info.hash].first;
			break;
		default:
			blocks[info.blk_idx].releasechunk(info.hash);
			break;
	}
}

void silofs::segtoblkidx(int idx)
{
	for (auto &&chk : segbuf.chunks)
	{
		blocks[idx].addchunk(chk.second.second, chk.second.first);
	}

	for (auto &&elem : pendings)
	{
		auto &&file = elem.second;
		while (file.lastseg < file.chunks.size() && 
			   file.chunks[file.lastseg].blk_idx == chunk_info::segbuf)
		{
			file.chunks[file.lastseg++].blk_idx = idx;
			--file.left;
		}
	}

	segbuf = {};
}

void silofs::segtoblkbuf()
{
	auto segitr = segbuf.chunks.begin();
	auto blkitr = blkbuf.chunks.begin();
	auto segend = segbuf.chunks.end();
	auto blkend = blkbuf.chunks.end();

	while (segitr != segend)
	{
		if (blkitr == blkend || segitr->first < blkitr->first)
		{
			blkbuf.chunks.insert(move(*segitr++));
		}
		else if (segitr->first > blkitr->first)
		{
			blkitr++;
		}
		else
		{
			blkitr->second.first += segitr->second.first;
			blkitr++;
			segitr++;
		}
	}

	for (auto &&elem : pendings)
	{
		auto &&file = elem.second;
		while (file.lastseg < file.chunks.size() && 
			   file.chunks[file.lastseg].blk_idx == chunk_info::segbuf)
		{
			file.chunks[file.lastseg++].blk_idx = chunk_info::blkbuf;
		}
	}

	segbuf = {};
}

void silofs::flushblkbuf()
{
	string dir = base + to_string(lastblock);
	if (mkdir(dir.c_str(), 0755) == -1)
	{
		fprintf(stderr, "creating dir %s failed\n", dir.c_str());
		exit(1);
	}
	blocks.emplace_back(dir.c_str());

	for (auto &&repid : blkbuf.segments)
	{
		shtbl.insert(repid, lastblock);
	}

	for (auto &&elem : blkbuf.chunks)
	{
		blocks[lastblock].addchunk(elem.second.second, elem.second.first);
	}

	for (auto &&elem : pendings)
	{
		auto &&file = elem.second;
		for (bool flag = true; file.lastblk < file.chunks.size() && flag;)
		{
			switch (file.chunks[file.lastblk].blk_idx)
			{
			case chunk_info::pending:
				flag = false;
				break;
			case chunk_info::blkbuf:
				file.chunks[file.lastblk].blk_idx = lastblock;
				--file.left;
			default:
				file.lastblk++;
				break;
			}
		}
	}

	blkbuf = {};

	lastblock++;
}

void silofs::flushpending()
{
	for (auto itr = pendings.begin(); itr != pendings.end();)
	{
		auto &&pending = itr->second;
		if (pending.left == 0)
		{
			string meta = base + volume_dir + itr->first;

			int fd = open(meta.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0755);
			if (fd == -1)
			{
				int err = errno;
				fprintf(stderr, "flush: %s open fail with errno %d\n", meta.c_str(), err);
				exit(1);
			}

			fprintf(stderr, "flush header size %jd\n", pending.header.size);

			::write(fd, &pending.header, sizeof(pending.header));

			for (auto &&info : pending.chunks)
			{
				::write(fd, &info, sizeof(info));
			}

			close(fd);
			itr = pendings.erase(itr);
		}
		else
		{
			++itr;
		}
	}
}

