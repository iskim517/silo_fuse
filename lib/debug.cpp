#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <execinfo.h>
#include "debug.h"

using namespace std;

void print_backtrace()
{
	fprintf(stderr, "Backtrace:\n");

	void *buf[10];
	int ret = backtrace(buf, 10);
	auto strings = backtrace_symbols(buf, ret);
	if (strings == nullptr)
	{
		fprintf(stderr, "Backtrace failed with errno %d\n", errno);
		return;
	}

	for (int i = 0; i < ret; i++)
	{
		printf("%s\n", strings[i]);
	}

	free(strings);
}

void safe_write(int fd, const void *buf, size_t len)
{
	auto ret = write(fd, buf, len);
	if (ret != len)
	{
		fprintf(stderr, "write failed with errno %d\n", errno);

		print_backtrace();
		exit(1);
	}
}

bool safe_read(int fd, void *buf, size_t len, bool must_success)
{
	auto ret = read(fd, buf, len);
	if (ret == 0 && must_success == false) return false;
	else if (ret != len)
	{
		fprintf(stderr, "read failed with errno %d\n", errno);

		print_backtrace();
		exit(1);
	}

	return true;
}

