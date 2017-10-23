#include <utility>
#include "libsilo.h"

bool do_chunking(const void *buf, std::size_t size, std::vector<std::size_t>& out)
{
    unsigned int window = 0;
    if(!out.empty()) return false;
    const char *it_l = static_cast<const char *>(buf), *it_r = it_l,
                *it_end = it_l + size;
    while(it_r != it_end)
    {
        window = (window * 17) + (unsigned char) (*it_r);
        it_r++;
        if(!(window % CHUNK_MOD) && it_r-it_l >= CHUNK_MIN_SZ
                || it_r - it_l >= CHUNK_MAX_SZ)
        {
            out.emplace_back(it_r - static_cast<const char *>(buf));
            window = 0;
            it_l = it_r;
        }
    }
    if(it_l != it_end)
    {
        out.emplace_back(it_end - static_cast<const char *>(buf));
    }
    return true;
}
