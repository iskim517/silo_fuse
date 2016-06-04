#pragma once

#include <string>
#include <array>
#include "chunk.h"

namespace silo
{
	using std::string;

	class block
	{
	private:
		string basedir;
	public:
		explicit block(const char *dir);
		block(const block &) = delete;
		block(block &&) = default;
		block &operator=(const block &) = delete;
		block &operator=(block &&) = default;

		chunk getchunk(const md5val &hash);
		void addchunk(const chunk &chk, uint64_t initref);
		void releasechunk(const md5val &hash);
	};
}
