#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <openssl/md5.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "shtable.h"
#include "libsilo.h"
#include <errno.h>
#include <fstream>
#include <istream>
#include <iterator>
using namespace std;

char buf[65536];
shtable sht;
unsigned int num_chunk, num_dup;
unsigned long long size_dup, size_org;

void dedup(const std::string &d)
{
    std::vector<size_t> chunked;
    do_chunking(d.c_str(), d.size(), chunked);
    num_chunk += chunked.size();
    size_org += d.size();
    size_t left = 0;
    for(size_t c : chunked)
    {
        size_t sz = c - left;
        std::array<unsigned char,16> hash;
        MD5((unsigned char*)(d.c_str()+left),sz,&hash[0]);
        left=c;
        int bnum;
        if (sht.find(bnum, hash))
        {
            num_dup++;
            size_dup += sz;
        }
        else sht.insert(hash, 1);
    }
}

void createdata(DIR* d, std::string addr)
{
    struct dirent* e;
    struct stat st;
    while((e = readdir(d)) != NULL)
    {
        if(!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        sprintf(buf,"%s/%s",addr.c_str(),e->d_name);
        lstat(buf, &st);
        if( S_ISDIR(st.st_mode))
        {
            DIR* dc = opendir(buf);
            createdata(dc, buf);
            closedir(dc);
        }
        else if( S_ISREG(st.st_mode))
        {
            ifstream in(buf);
            if (in.is_open() == false)
            {
                printf("%s fail\n", buf);
                exit(1);
            }
            std::string tmp{istream_iterator<char>{in}, istream_iterator<char>{}};
            dedup(tmp);
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1) return 0;
    DIR *f = opendir(argv[1]);
    createdata(f, argv[1]);
    closedir(f);
    printf("number of chunks:     %u\n", num_chunk);
    printf("number of dup chunks: %u\n", num_dup);
    printf("total memory if no dedup: %lluB\n", size_org);
    printf("memory saved if optimal:  %lluB\n", size_dup);
}
