#ifndef __BPT_H__
#define __BPT_H__

#include <stdint.h>

typedef int64_t      bpt_key_t;
typedef char *       bpt_value_t;
typedef const char * c_bpt_value_t;

int open_db(const char * pathname);
int insert(bpt_key_t key, c_bpt_value_t value);
bpt_value_t find(bpt_key_t key);
int delete(bpt_key_t key);

#endif