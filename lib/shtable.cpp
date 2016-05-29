#include "shtable.h"

size_t shtable::make_val(int blocknum, int segmentnum)
{
    return ((size_t)blocknum << 8) + segmentnum & 0xff;
}

shtable::shtable(){}

bool shtable::insert(std::array<char,16> key, int blocknum, int segmentnum)
{
    size_t val = make_val(blocknum, segmentnum);
    return btr.insert(key, val);
}

bool shtable::find(int& blocknum, int& segmentnum, std::array<char, 16> key)
{
    size_t val = btr.find(key);
    if(!val) return false;
    blocknum = val >> 8;
    segmentnum = val & 0xff;
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
