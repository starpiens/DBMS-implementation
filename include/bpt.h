#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int open_db(char * pathname);
int insert(int64_t key, char * value);
char * find(int64_t key);
int delete(int64_t key);
