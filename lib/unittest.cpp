#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<chrono>
#include<array>
#include<vector>
#include<set>
#include "btree.h"
#include "libsilo.h"
using namespace std;

#define btree_test_sz 1000000

btree btr;
std::array<unsigned char,16>key[btree_test_sz];
int val[btree_test_sz];
char savedir[100] = "srtest";


void btree_init()
{
    for(int i=0;i<btree_test_sz;i++)
    {
        key[i][13] = (i>>16)%256;
        key[i][14] = (i>>8)%256;
        key[i][15] = i%256;
        for(int j=0;j<13;j++)
        {
            key[i][j] = rand()%256;
        }
        val[i] = rand();
    }
}

bool btree_insert()
{
    bool ret = true;
    for(int i=0;i<btree_test_sz;i++)
    {
        ret &= btr.insert(key[i],val[i]);
        if(!ret)
        {
            printf("error on %d\n", i);
            return false;
        }
    }
    return ret;
}

bool set_insert()
{
    bool ret = true;
    auto comp = [](auto &a, auto &b){return keyval_comp(a, b); };
    set<std::array<unsigned char, 16>, decltype(comp)> st(comp);
    for (int i = 0; i < btree_test_sz; i++)
    {
        ret &= st.insert(key[i]).second;
        if (!ret)
        {
            printf("error on %d\n", i);
            return false;
        }
    }
    return ret;
}

bool btree_find()
{
    bool ret = true;
    int r;
    for(int i= btree_test_sz-1;i>=0;i--)
    {
        r = btr.find(key[i]);
        ret &= r == val[i];
    }
    return ret;
}

bool btree_save()
{
    return btr.save(savedir);
}

bool btree_load()
{
    btree lbtr;
    lbtr.load(savedir);
    bool ret = true;
    int r;
    for(int i= btree_test_sz-1;i>=0;i--)
    {
        r = lbtr.find(key[i]);
        ret &= r == val[i];
        if(!ret)
        {
            printf("err on %d\n",i);
            return false;
        }
    }
    return ret;
}

long long now()
{
    static auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t).count();
}

void test_btree()
{
    bool yes;
    auto t = now();
    puts("testing btree");
    btree_init();
    printf("initializing data of size %d in %lld millisecond\n", btree_test_sz, now() - t);

    t = now();
    yes = btree_insert();
    if(yes) printf("insert in %lld millisecond\n", now() - t);
    else puts("error on btree insert");
    
    t = now();
    yes = set_insert();
    if(yes) printf("set insert in %lld millisecond\n", now() - t);
    else puts("error on set insert");

    t = now();
    yes = btree_find();
    if(yes) printf("find in %lld millisecond\n", now() - t);
    else puts("error on btree find");

    t = now();
    yes = btree_save();
    if(yes) printf("save in %lld millisecond\n", now() - t);
    else puts("error on btree save");

    t = now();
    yes = btree_load();
    if(yes) printf("load and check in %lld millisecond\n", now() - t);
    else puts("error on btree load");

    remove(savedir);
}

#define lib_file_test_sz 209715200 //200MB
std::vector<char> libfile;
std::vector<size_t> chunks;
void rand_file_init()
{
    for(int i=0;i<lib_file_test_sz;i++)
    {
        char x = rand()%256;
        libfile.push_back(x);
    }
}

bool lib_chunk()
{
    return do_chunking(&libfile[0], libfile.size(), chunks);
}

bool lib_chunk_check()
{
    for (size_t i = 1; i < chunks.size(); i++)
    {
        if (chunks[i] <= chunks[i-1]) return false;
    }
    return chunks.back() == libfile.size();
}

void lib_chunk_analysis()
{
    puts("begin chunking analysis");
    printf("number of chunks : %lu\n", chunks.size());
    printf("average size of chunk : %lfB\n", (double) lib_file_test_sz / (double) chunks.size());
    unsigned int mx = 0, mn = 2147483647;
    unsigned int last = 0;
    for(int i=0;i<chunks.size() - 1;i++)
    {
        unsigned int chunksize = chunks[i] - last;
        if(chunksize > mx) mx = chunksize;
        if(chunksize < mn) mn = chunksize;

        last = chunks[i];
    }
    if(chunks.back() - last > mx) mx = chunks.back() - last;
    printf("largest chunk : %uB\n", mx);
    printf("smallest chunk : %uB\n", mn);
}


void test_lib()
{
    bool yes;
    auto t = now();
    puts("testing lib");
    rand_file_init();
    printf("created random file of size %dKB in %lld millisecond\n", lib_file_test_sz/1024, now() - t);

    t = now();
    yes = lib_chunk();
    if(yes) printf("chunking in %lld millisecond\n", now() - t);
    else puts("error on lib do_chunking");

    t = now();
    yes = lib_chunk_check();
    if(yes) printf("check chunks in %lld millisecond\n", now() - t);
    else puts("error on lib do_chunking check");

    lib_chunk_analysis();
}

int main(int argc, char* argv[])
{
    bool do_btree, do_lib;
    srand(now());
    if(argc == 1)
    {
        do_btree = true;
        do_lib = true;
    }
    else
    {
        for(int i=1;i<argc;i++)
        {
            if(!strcmp(argv[i],"btree"))
            {
                do_btree = true;
            }
            else if(!strcmp(argv[i],"lib"))
            {
                do_lib = true;
            }
            else
            {
                printf("WRONG ARGUMENT : %s\n",argv[i]);
                puts("List of arguments: btree, lib");
            }
        }
    }
    if(do_btree) test_btree();
    if(do_lib) test_lib();
}
