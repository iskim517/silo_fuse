#include <vector>
#include <string.h>
#include <zlib.h>
#include <limits>
#include <openssl/md5.h>
#include "chunk.h"
#include "lib/libsilo.h"
using namespace std;
using namespace silo;

chunk chunk::frombuffer(const void *buf, size_t len)
{
	chunk ret;
	if (len > CHUNK_SZ)
	{
		fprintf(stderr, "chunk size %jd is larger than %d\n", len, CHUNK_SZ);
		exit(1);
	}
	vector<unsigned char> padded(CHUNK_SZ);
	memcpy(&padded[0], buf, len);

	len = CHUNK_SZ;

	MD5(&padded[0], len, &ret.hash[0]);

	uLongf destLen = compressBound(len);
	ret.blob.resize(destLen);
	if (compress(reinterpret_cast<Bytef *>(&ret.blob[0]), &destLen, &padded[0], len)
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

	size_t rawsize = CHUNK_SZ;
	vector<char> ret(rawsize);

	uLongf destLen = rawsize;
	int ret2 = uncompress(reinterpret_cast<Bytef *>(&ret[0]), &destLen,
		reinterpret_cast<const Bytef *>(&blob[0]), blob.size());
	if (ret2 != Z_OK)
	{
		if (ret2 == Z_MEM_ERROR) fprintf(stderr, "Z_MEM_ERROR\n");
		else if (ret2 == Z_BUF_ERROR) fprintf(stderr, "Z_BUF_ERROR\n");
		else if (ret2 == Z_DATA_ERROR) fprintf(stderr, "Z_DATA_ERROR\n");
		else fprintf(stderr, "Z_UNKNOWN %d\n", ret2);

		fprintf(stderr, "chunk unzip error\n");
		for (int i = 0; i < 16; i++)
		{
			fprintf(stderr, "%02x", hash[i]);
		}
		fprintf(stderr, "\n");
		exit(1);
	}

	if (rawsize != destLen)
	{
		fprintf(stderr, "chunk rawsize != destLen\n");
		exit(1);
	}

	return ret;
}
