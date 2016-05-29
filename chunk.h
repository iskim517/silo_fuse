#pragma once

#include <vector>
#include <cstdint>

namespace silo
{
	using std::uint16_t;
	using std::uint64_t;
	using std::vector;
	using md5val = std::array<unsigned char, 16>;

	enum class comptype : char
	{
		raw,
		gzip
	};

	struct chunk_file_header
	{
		uint64_t refcount;
		comptype type;
	} __attribute((packed));

	struct chunk
	{
		comptype type;
		md5val hash;
		vector<char> blob;
	};
}
