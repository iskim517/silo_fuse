#include <utility>
#include "libsilo.h"
#include <algorithm>
using std::min;

bool do_chunking(const void *buf, std::size_t size, std::vector<std::size_t>& out)
{
	if(!out.empty()) return false;

	size_t now = CHUNK_SZ;
	while (now < size)
	{
		out.push_back(now);
		now += CHUNK_SZ;
	}

	out.push_back(size);

	return true;
}
