#pragma once

void print_backtrace();
void safe_write(int fd, const void *buf, size_t len);
bool safe_read(int fd, void *buf, size_t len, bool must_success = false);
