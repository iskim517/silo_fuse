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

char buf[65536];
unsigned char buf2[65536];
shtable sht;
std::string data;

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
            FILE *f = fopen(buf,"r");
            if (f == nullptr)
            {
                printf("%s fail %d\n", buf, errno);
                exit(1);
            }
            while(fread(buf,sizeof(char),sizeof(buf),f))
            {
                data += std::string(buf);
            }
            fclose(f);
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1) return 0;
    DIR *f = opendir(argv[1]);
    createdata(f, argv[1]);
    closedir(f);
    std::vector<size_t> chunked;
    do_chunking(data.c_str(), data.size(), chunked);
    unsigned int num_chunk = chunked.size(), num_dup = 0;
    unsigned long long size_dup = 0, size_org = data.size();
    size_t left = 0;
    for(size_t c : chunked)
    {
        size_t sz = c - left;
        memset(buf2,0,sizeof(buf2));
        for(int i=0;left<c;i++)buf2[i] = (unsigned char) data[left++];
        std::array<unsigned char,16> hash;
        MD5(buf2,sz,&hash[0]);
        int bnum;
        if (sht.find(bnum, hash))
        {
            num_dup++;
            size_dup += sz;
        }
        else sht.insert(hash, 1);
    }
    printf("number of chunks:     %u\n", num_chunk);
    printf("number of dup chunks: %u\n", num_dup);
    printf("total memory if no dedup: %lluB\n", size_org);
    printf("memory saved if optimal:  %lluB\n", size_dup);
}
