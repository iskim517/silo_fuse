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
        std::array<char,16> key;
        size_t val;
        bool operator <(const keyval & other) const
        {
            return key < other.key;
        }
        bool operator <(const std::array<char,16> & other) const
        {
            return key < other;
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
    bool insert_rec(std::array<char,16> key, size_t val);
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
    bool insert(std::array<char,16> key, size_t val);
    size_t find(std::array<char,16> key);
    bool save(char* dir);
    bool load(char* dir);
};
#endif
