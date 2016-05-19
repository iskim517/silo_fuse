#include<stdio.h>
#include<chrono>
#include<array>
#include "btree.h"

#define btree_test_sz 1000000

btree btr;
std::array<char,16>key[btree_test_sz];
size_t val[btree_test_sz];
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
            key[i][j] = ((i*3+5)*(j*7+11))%256;
        }
        val[i] = (i*997 + 100003);
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

bool btree_find()
{
    bool ret = true;
    size_t r;
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
    size_t r;
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

int main()
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
