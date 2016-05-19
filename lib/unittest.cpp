#include<stdio.h>
#include<chrono>
#include<array>
#include "btree.h"

btree btr;
std::array<char,16>key[1000000];
size_t val[1000000];
char savedir[100] = "srtest";


bool btree_init()
{
    for(int i=0;i<1000000;i++)
    {
        key[i][13] = (i>>16)%256;
        key[i][14] = (i>>8)%256;
        key[i][15] = i%256;
        for(int j=0;j<13;j++)
        {
            key[i][j] = ((i*3+5)*(j*7+11))%256;
        }
        val[i] = (i*997 + 100003);
    }
}

bool btree_insert()
{
    bool ret = true;
    for(int i=0;i<1000000;i++)
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

bool btree_find()
{
    bool ret = true;
    size_t r;
    for(int i=999999;i>=0;i--)
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

long long now()
{
    static auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t).count();
}

int main()
{
    bool yes;
    auto t = now();
    puts("testing btree");
    btree_init();
    printf("initializing btree in %lld millisecond\n", now() - t);

    t = now();
    yes = btree_insert();
    if(yes) printf("insert 1000000 in %lld millisecond\n", now() - t);
    else puts("error on btree insert");

    t = now();
    yes = btree_find();
    if(yes) printf("find 1000000 in %lld millisecond\n", now() - t);
    else puts("error on btree find");

    t = now();
    yes = btree_save();
    if(yes) printf("save 1000000 in %lld millisecond\n", now() - t);
    else puts("error on btree save");
}
