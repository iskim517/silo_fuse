#include<stdio.h>
#include<chrono>
#include<array>
#include "btree.h"

btree btr;
std::array<char,16>key[1000000];
size_t val[1000000];

bool btree_init()
{
    int a=0,b=0,c=0;
    for(int i=0;i<1000000;i++)
    {
        key[i][0] = a;
        key[i][1] = b;
        key[i][2] = c++;
        if(c&256)
        {
            b++;
            c = 0;
        }
        if(b&256)
        {
            a++;
            b = 0;
        }
        for(int j=3;j<16;j++)
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

long long now()
{
    static auto t = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t).count();
}

int main()
{
    bool yes;
    btree_init();
    puts("testing btree");
    auto t = now();
    yes = btree_insert();
    if(yes) printf("insert 1000000 in %lld millisecond\n", now() - t);
    else puts("error on btree insert");

    t = now();
    yes = btree_find();
    if(yes) printf("find 1000000 in %lld millisecond\n", now() - t);
    else puts("error on btree find");
}
