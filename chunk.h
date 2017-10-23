#pragma once

#include <vector>
#include <cstdint>
#include <array>

namespace silo
{
    using std::uint64_t;
    using std::uint16_t;
    using std::vector;
    using md5val = std::array<unsigned char, 16>;

    enum class comptype : char
    {
        raw,
        gzip
    };

    struct chunk
    {
        comptype type;
        md5val hash;
        uint16_t rawsize;
        vector<char> blob;

        static chunk frombuffer(const void *buf, size_t len);
        vector<char> unzip() const;
    };
}
