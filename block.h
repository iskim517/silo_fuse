#pragma once

#include <string>
#include <array>
#include <map>
#include "chunk.h"

namespace silo
{
    using std::string;
    using std::map;
    using std::pair;

    struct chunk_file_header
    {
        uint64_t refcount;
        uint16_t compsize;
        uint16_t rawsize;
        md5val hash;
        comptype type;
    } __attribute((packed));

    class block
    {
    private:
        string basedir;
        bool find_chunk(int fd, const md5val &hash, chunk_file_header &ret);
        void defragment(int fd, const string &name);
    public:
        explicit block(const char *dir);
        block(const block &) = delete;
        block(block &&) = default;
        block &operator=(const block &) = delete;
        block &operator=(block &&) = default;

        chunk getchunk(const md5val &hash);
        void addchunk(const chunk &chk, uint64_t initref);
        void create_with_chunks(const map<md5val, pair<uint64_t, chunk>> &chks);
        void releasechunk(const md5val &hash);
    };
}
