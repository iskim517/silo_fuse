#ifndef _LIBSILO_H_
#define _LIBSILO_H_

#define CHUNK_MOD 277
#define CHUNK_MIN_SZ 4096
#define CHUNK_MAX_SZ 16384

#include <vector>
bool do_chunking(const void *in, std::size_t len, std::vector<std::size_t>& out);

#endif
