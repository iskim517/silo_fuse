#ifndef _LIBSILO_H_
#define _LIBSILO_H_

#define CHUNK_MOD 4111

#include <vector>
bool do_chunking(const void *in, std::size_t len, std::vector<std::size_t>& out);

#endif
