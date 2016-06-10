#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include <set>
#include <sys/stat.h>
#include "lib/shtable.h"
#include "block.h"
#include "chunk.h"

namespace silo
{
	using std::string;
	using std::set;
	using std::map;
	using std::uint64_t;
	using std::uint32_t;
	using std::pair;

	constexpr int SEGMENTS_PER_BLOCK = 256;
	constexpr int SEGMENT_SIZE = 2097152;

	struct chunk_info
	{
		static const int pending = -3;
		static const int segbuf = -2;
		static const int blkbuf = -1;
		int blk_idx;
		md5val hash;
	} __attribute((packed));

	struct file_header
	{
		mode_t mod;
		uid_t uid;
		gid_t gid;
		time_t atime, mtime, ctime;
		off_t size;
		off_t chunks;
	} __attribute((packed));

	struct pending_file
	{
		file_header header;
		vector<chunk_info> chunks;
		size_t lastseg, lastblk;
		size_t left;
	};

	struct segment_buffer
	{
		uint32_t size = 0;
		map<md5val, pair<uint64_t, chunk>> chunks;

		void deserialize(const char *file);
		void serialize(const char *file);
	};

	struct block_buffer
	{
		set<md5val> segments;
		map<md5val, pair<uint64_t, chunk>> chunks;

		void deserialize(const char *file);
		void serialize(const char *file);
	};

	class silofs
	{
	private:
		static constexpr const char *segbuf_file = "segbuf";
		static constexpr const char *blkbuf_file = "blockbuf";
		static constexpr const char *shtable_file = "shtable";
		static constexpr const char *pending_name = "pending";
		static constexpr const char *volume_dir = "volume/";

		string base;
		block_buffer blkbuf;
		segment_buffer segbuf;
		vector<block> blocks;
		map<string, pending_file> pendings;
		shtable shtbl;

		int lastblock;
		
		vector<char> readchunk(chunk_info info);
		void releasechunk(chunk_info info);
		void segtoblkidx(int idx);
		void segtoblkbuf();
		void flushblkbuf();
		void flushpending();
	public:
		explicit silofs(const char *basedir);
		~silofs();
		silofs(const silofs &) = delete;
		silofs &operator=(const silofs &) = delete;

		bool read(const char *file, vector<char> &ret);
		file_header header(const char *file);
		void write(const char *file, const void *dat, size_t size, file_header header);
		void writeheader(const char *file, file_header header);
		void remove(const char *file);
	};
}
