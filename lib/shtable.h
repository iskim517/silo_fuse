#pragma once
#ifndef _SHTABLE_H_
#define _SHTABLE_H_

#include "btree"

class shtable
{
private:
btree btr;
size_t make_val(int blocknum, int segmentnum);
public:
shtable();
bool insert(std::array<char,16> key, int blocknum, int segmentnum);
bool find(int& blocknum, int& segmentnum, std::array<char,16> key);
bool save(const char* dir);
bool load(const char* dir);
};

#endif
