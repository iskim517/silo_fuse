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
#include "lib/debug.h"

using namespace std;
using namespace silo;

static const char *meta = "meta";

block::block(const char *dir) : basedir(dir)
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

static char to_hex(unsigned char ch)
{
    if (ch < 10) return ch + '0';
    return ch + 'a' - 10;
}

static string to_postfix(const md5val &hash)
{
    return { to_hex(hash[0] >> 4), to_hex(hash[0] & 0xF), to_hex(hash[1] >> 7) };
}

bool block::find_chunk(int fd, const md5val &hash, chunk_file_header &ret)
{
    for (;;)
    {
        if (read(fd, &ret, sizeof(ret)) < sizeof(ret)) return false;
        if (ret.hash == hash) return true;
        lseek(fd, ret.rawsize, SEEK_CUR);
    }

    return false;
}

chunk block::getchunk(const md5val &hash)
{
    string name = basedir + to_postfix(hash);

    int fd = open(name.c_str(), O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "chunk %s not found %d\n", name.c_str(), errno);
        exit(1);
    }

    lseek(fd, 2, SEEK_SET);

    chunk_file_header chk;
    if (find_chunk(fd, hash, chk) == false || chk.refcount == 0)
    {
        fprintf(stderr, "chunk %s not found %d\n", to_string(hash).c_str(), errno);
        exit(1);
    }

    chunk ret{ chk.type, chk.hash, chk.rawsize, vector<char>(chk.compsize) };
    read(fd, &ret.blob[0], chk.compsize);
    close(fd);

    return ret;
}

void block::addchunk(const chunk &chk, uint64_t initref)
{
    string name = basedir + to_postfix(chk.hash);

    int fd = open(name.c_str(), O_RDWR);
    if (fd == -1)
    {
        create_with_chunks({{chk.hash, {initref, move(chk)}}});
        return;
    }

    lseek(fd, 2, SEEK_SET);

    chunk_file_header header;
    if (find_chunk(fd, chk.hash, header) == false)
    {
        header.refcount = initref;
        header.compsize = chk.blob.size();
        header.rawsize = chk.rawsize;
        header.hash = chk.hash;
        header.type = chk.type;
        write(fd, &header, sizeof(header));
        write(fd, &chk.blob[0], chk.blob.size());
        close(fd);

        return;
    }

    header.refcount += initref;
    lseek(fd, -(off_t)sizeof(header), SEEK_CUR);
    write(fd, &chk, sizeof(header));

    if (header.refcount == initref)
    {
        lseek(fd, 0, SEEK_SET);
        uint16_t deleted;
        read(fd, &deleted, 2);
        --deleted;
        lseek(fd, 0, SEEK_SET);
        write(fd, &deleted, 2);
    }

    close(fd);
}

void block::create_with_chunks(const map<md5val, pair<uint64_t, chunk>> &chks)
{
    uint16_t last = 0;
    int fd = -1;

    for (auto &chk : chks)
    {
        if (last != (chk.first[0] << 1 | (chk.first[1] >> 7)) || fd == -1)
        {
            if (fd != -1) close(fd);
            last = chk.first[0] << 1 | (chk.first[1] >> 7);
            fd = open((basedir + to_postfix(chk.first)).c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0755);
            if (fd == -1)
            {
                fprintf(stderr, "block file %s open failed with errno %d\n",
                    (basedir + to_postfix(chk.first)).c_str(), errno);
                exit(1);
            }
            write(fd, &(const int &)0, 2);
        }

        chunk_file_header header{
            chk.second.first,
            chk.second.second.blob.size(),
            chk.second.second.rawsize,
            chk.first,
            chk.second.second.type
        };

        write(fd, &header, sizeof(header));
        write(fd, &chk.second.second.blob[0], chk.second.second.blob.size());
    }

    if (fd != -1) close(fd);
}

void block::releasechunk(const md5val &hash)
{
    string name = basedir + to_postfix(hash);

    int fd = open(name.c_str(), O_RDWR);
    if (fd == -1) return;

    uint16_t deleted;
    read(fd, &deleted, 2);

    chunk_file_header header;
    if (find_chunk(fd, hash, header) == false) return;
    if (header.refcount == 0) return;

    --header.refcount;
    lseek(fd, -(off_t)sizeof(header), SEEK_CUR);
    write(fd, &header, sizeof(header));

    if (header.refcount == 0)
    {
        ++deleted;

        if (deleted == 128)
        {
            defragment(fd, name);
            close(fd);
            return;
        }

        lseek(fd, 0, SEEK_SET);
        write(fd, &deleted, 2);
    }
    close(fd);
}

void block::defragment(int fd, const string &name)
{
    lseek(fd, 0, SEEK_SET);
    write(fd, &(const int &)0, 2);

    off_t last = 2;
    bool passed = false;

    for (;;)
    {
        chunk_file_header header;
        if (read(fd, &header, sizeof(header)) < sizeof(header)) break;

        if (header.refcount == 0)
        {
            passed = true;
            lseek(fd, header.compsize, SEEK_CUR);
            continue;
        }

        if (passed)
        {
            char buf[header.compsize];
            read(fd, buf, header.compsize);
            off_t old = lseek(fd, 0, SEEK_CUR);
            lseek(fd, last, SEEK_SET);
            write(fd, &header, sizeof(header));
            write(fd, buf, header.compsize);
            lseek(fd, old, SEEK_SET);
        }

        last += sizeof(header) + header.compsize;
    }

    ftruncate(fd, last);
}
