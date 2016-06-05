#include <utility>
#include "libsilo.h"

bool do_chunking(const void *buf, std::size_t size, std::vector<std::vector<char>>& out)
{
    unsigned int window = 0;
    if(!out.empty()) return false;
    const char *it_l = static_cast<const char *>(buf), *it_r = it_l,
				*it_end = it_l + size;
    while(it_r != it_end)
    {
        window <<= 8;
        window += (unsigned char) (*it_r) + 17;
        it_r++;
        if(!(window%CHUNK_MOD))
        {
            out.emplace_back(it_l, it_r);
            window = 0;
            it_l = it_r;
        }
    }
    if(it_l != it_end)
    {
        out.emplace_back(it_l, it_end);
    }
    return true;
}
