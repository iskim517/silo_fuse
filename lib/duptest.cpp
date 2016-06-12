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
unsigned int num_chunk, num_dup;
unsigned long long size_dup, size_org;

void dedup(std::string d)
{
    std::vector<size_t> chunked;
    do_chunking(d.c_str(), d.size(), chunked);
    num_chunk += chunked.size();
    size_org += d.size();
    size_t left = 0;
    for(size_t c : chunked)
    {
        size_t sz = c - left;
        memset(buf2,0,sizeof(buf2));
        for(int i=0;left<c;i++)buf2[i] = (unsigned char) d[left++];
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
            FILE *f = fopen(buf,"r");
            if (f == nullptr)
            {
                printf("%s fail %d\n", buf, errno);
                exit(1);
            }
            std::string tmp = "";
            while(fread(buf,sizeof(char),sizeof(buf),f))
            {
                tmp += std::string(buf);
            }
            dedup(tmp);
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
    printf("number of chunks:     %u\n", num_chunk);
    printf("number of dup chunks: %u\n", num_dup);
    printf("total memory if no dedup: %lluB\n", size_org);
    printf("memory saved if optimal:  %lluB\n", size_dup);
}
