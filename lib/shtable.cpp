#include "shtable.h"

shtable::shtable(){}

bool shtable::insert(std::array<char,16> key, int val)
{
    return btr.insert(key, val);
}

bool shtable::find(int& blocknum, std::array<char, 16> key)
{
    int val = btr.find(key);
    if(!val) return false;
    blocknum = val;
    return true;
}

bool shtable::save(const char* dir)
{
    return btr.save(dir);
}

bool shtable::load(const char* dir)
{
    return btr.load(dir);
}
