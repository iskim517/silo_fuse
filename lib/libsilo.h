#ifndef _LIBSILO_H_
#define _LIBSILO_H_

#define CHUNK_MOD 4111

#include<vector>
bool do_chunking(std::vector<char> in, std::vector<std::vector<char>>& out);

#endif
