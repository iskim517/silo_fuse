#pragma once
#ifndef _SHTABLE_H_
#define _SHTABLE_H_

#include "btree.h"

class shtable
{
private:
btree btr;
public:
shtable();
bool insert(std::array<unsigned char,16> key, int val);
bool find(int& val, std::array<unsigned char,16> key);
bool save(const char* dir);
bool load(const char* dir);
};

#endif
