#pragma once
#ifndef _BTREE_H_
#define _BTREE_H_
#include<algorithm>
#include<vector>
#include<array>
#define BTREE_MAX_SZ 255
class btree
{
private:
    //struct of key value pair
    typedef struct keyval
    {
        std::array<unsigned char,16> key;
        int val;
        bool operator <(const keyval & other) const
        {
            if ((const uint64_t &)key[0] != (const uint64_t &)other.key[0])
                return (const uint64_t &)key[0] < (const uint64_t &)other.key[0];

            return (const uint64_t &)key[8] < (const uint64_t &)other.key[8];
        }
        bool operator <(const std::array<unsigned char,16> & other) const
        {
            if ((const uint64_t &)key[0] != (const uint64_t &)other[0])
                return (const uint64_t &)key[0] < (const uint64_t &)other[0];

            return (const uint64_t &)key[8] < (const uint64_t &)other[8];
        }
    } kv;
    std::vector<kv> node;
    std::vector<btree*> child;
    bool leaf;
    /*
    * split current node and use it as left child
    * pass center key and right child to its parent
    */
    void split(btree* &right, kv& center);
    /*
    * insert function for child node
    */
    bool insert_rec(std::array<unsigned char,16> key, int val);
    bool serialize(FILE* &f);
    bool deserialize(FILE* &f);
public:
    btree();
    btree(std::vector<kv> init_node);
    btree(std::vector<kv> init_node, std::vector<btree*> init_child);
    ~btree();
    size_t size();
    /*
    * insert function for root node
    */
    bool insert(std::array<unsigned char,16> key, int val);
    int find(std::array<unsigned char,16> key);
    bool save(const char* dir);
    bool load(const char* dir);
};
#endif
