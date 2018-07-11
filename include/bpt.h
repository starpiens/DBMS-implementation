#ifndef __BPT_H__
#define __BPT_H__

#include <stdint.h>

/* Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return 0. Otherwise, return non-zero value.
 */
int open_db(const char * pathname);

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int insert(int64_t key, const char * value);

/* Find the record containing input ‘key’.
 * If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
char * find(int64_t key);

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(int64_t key);

#endif