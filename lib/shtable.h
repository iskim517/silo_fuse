#pragma once
#ifndef _SHTABLE_H_
#define _SHTABLE_H_

#include "btree"

class shtable
{
private:
btree btr;
public:
shtable();
bool insert(std::array<char,16> key, int val);
bool find(int& val, std::array<char,16> key);
bool save(const char* dir);
bool load(const char* dir);
};

#endif
