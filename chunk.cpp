#include <vector>
#include <string.h>
#include <zlib.h>
#include <limits>
#include <openssl/md5.h>
#include "chunk.h"
using namespace std;
using namespace silo;

chunk chunk::frombuffer(const void *buf, size_t len)
{
	chunk ret;

	if (len > 1024 * 1024)
	{
		fprintf(stderr, "chunk::frombuffer too large\n");
		exit(1);
	}

	ret.rawsize = len;
	MD5(static_cast<const unsigned char *>(buf), len, &ret.hash[0]);

	uLongf destLen = compressBound(len);
	ret.blob.resize(destLen);
	if (compress(reinterpret_cast<Bytef *>(&ret.blob[0]), &destLen, static_cast<const Bytef *>(buf), len)
		!= Z_OK)
	{
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	if (destLen < len)
	{
		ret.blob.resize(destLen);
		ret.type = comptype::gzip;
	}
	else
	{
		ret.blob.resize(len);
		memcpy(&ret.blob[0], buf, len);
		ret.type = comptype::raw;
	}

	return ret;
}

vector<char> chunk::unzip() const
{
	if (type == comptype::raw) return blob;

	vector<char> ret(rawsize);

	uLongf destLen = rawsize;
	if (uncompress(reinterpret_cast<Bytef *>(&ret[0]), &destLen, 
		reinterpret_cast<const Bytef *>(&blob[0]), blob.size()) != Z_OK)
	{
		fprintf(stderr, "chunk unzip error\n");
		exit(1);
	}

	if (rawsize != destLen)
	{
		fprintf(stderr, "chunk rawsize != destLen\n");
		exit(1);
	}

	return ret;
}
