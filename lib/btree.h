
aendif once
#ifndef _BTREE_H_
#define _BTREE_H_
#include<algorithm>
#include<vector>
#define BTREE_MAX_SZ 63

template<typename T1, typename T2>
class Btree
{
private:
    //struct of key value pair
    typedef struct keyval
    {
        T1 key;
        T2 val;
        bool operator <(const keyval & other) const
        {
            return key < other.key;
        }
        bool operator <(const int & other) const
        {
            return key < other;
        }
    } kv;
    std::vector<kv> node;
    std::vector<Btree*> child;
    bool leaf;
    /*
    * split current node and use it as left child
    * pass center key and right child to its parent
    */
    void split(Btree* &right, kv& center)
    {
        int cidx = BTREE_MAX_SZ >> 1;
        center = node[cidx];
        std::vector<kv> rn(node.begin() + cidx + 1, node.end());
        if (leaf)
        {
            right = new Btree(rn);
        }
        else
        {
            std::vector<Btree*> rc(child.begin() + cidx + 1, child.end());
            right = new Btree(rn, rc);
            child._Pop_back_n(rc.size());
        }
        node._Pop_back_n(rn.size() + 1);
        return;
    }
    /*
    * insert function for child node
    */
    bool insert_rec(T1 key, T2 val)
    {
        auto it = std::lower_bound(node.begin(), node.end(), key);
        if (it != node.end() && it->key == key) return false;
        if (leaf)
        {
            kv tmp;
            tmp.key = key;
            tmp.val = val;
            node.insert(it, tmp);
            return true;
        }
        else
        {
            size_t d = std::distance(node.begin(), it);
            while (child.size() <= d)
            {
                Btree *ntree;
                ntree = new Btree();
                child.push_back(ntree);

            }
            if (!child[d])
            {
                child[d] = new Btree();
            }
            if (child[d]->size() >= BTREE_MAX_SZ)
            {
                Btree *r;
                kv center;
                child[d]->split(r, center);
                node.insert(it, center);
                child.insert(child.begin() + d + 1, r);
                if (center.key == key) return false;
                if (center.key < key) d++;
            }
            return child[d]->insert_rec(key, val);
        }
    }
public:
    Btree()
    {
        leaf = true;
    }
    Btree(std::vector<kv> initial_node)
    {
        leaf = true;
        node = initial_node;
    }
    Btree(std::vector<kv> initial_node, std::vector<Btree*> initial_child)
    {
        leaf = false;
        node = initial_node;
        child = initial_child;
    }
    ~Btree()
    {
        node.clear();
        child.clear();
    }
    size_t size()
    {
        return node.size();
    }
    /*
    * insert function for root node
    */
    bool insert(T1 key, T2 val)
    {
        if (node.empty())
        {
            kv tmp;
            tmp.key = key;
            tmp.val = val;
            node.push_back(tmp);
            return true;
        }
        auto it = std::lower_bound(node.begin(), node.end(), key);
        if (it != node.end() && it->key == key) return false;
        if (leaf)
        {
            kv tmp;
            tmp.key = key;
            tmp.val = val;
            node.insert(it, tmp);

            if (node.size() >= BTREE_MAX_SZ)
            {
                leaf = false;
                Btree* l, *r;
                int cidx = BTREE_MAX_SZ >> 1;
                kv center = node[cidx];
                std::vector<kv> ln(node.begin(), node.begin() + cidx);
                std::vector<kv> rn(node.begin() + cidx + 1, node.end());
                l = new Btree(ln);
                r = new Btree(rn);
                node.clear();
                node.push_back(center);
                child.push_back(l);
                child.push_back(r);
            }
            return true;
        }
        else
        {
            if (node.size() >= BTREE_MAX_SZ)
            {
                leaf = false;
                Btree* l, *r;
                int cidx = BTREE_MAX_SZ >> 1;
                kv center = node[cidx];
                std::vector<kv> ln(node.begin(), node.begin() + cidx);
                std::vector<Btree*> lc(child.begin(), child.begin() + cidx + 1);
                std::vector<kv> rn(node.begin() + cidx + 1, node.end());
                std::vector<Btree*> rc(child.begin() + cidx + 1, child.end());
                l = new Btree(ln, lc);
                r = new Btree(rn, rc);
                node.clear();
                node.push_back(center);
                child.clear();
                child.push_back(l);
                child.push_back(r);
            }
            it = std::lower_bound(node.begin(), node.end(), key);
            size_t d = std::distance(node.begin(), it);
            if (child[d]->size() >= BTREE_MAX_SZ)
            {
                Btree *r;
                kv center;
                child[d]->split(r, center);
                node.insert(it, center);
                child.insert(child.begin() + d + 1, r);
                if (center.key == key) return false;
                if (center.key < key) d++;
            }
            return child[d]->insert_rec(key, val);
        }
    }
    T2 find(T1 key)
    {
        auto it = std::lower_bound(node.begin(), node.end(), key);
        if (it != node.end() && it->key == key) return it->val;
        if (leaf) return NULL;
        int d = std::distance(node.begin(), it);
        return child[d]->find(key);
    }
};
#endif
