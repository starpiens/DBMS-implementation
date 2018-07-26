#ifndef __BPT_H__
#define __BPT_H__

#include <stdint.h>

int open_db(const char * pathname);
int insert(int64_t key, const char * value);
char * find(int64_t key);
int delete(int64_t key);

#endif