#ifndef __FILE_H__
#define __FILE_H__

#include "sys/types.h"
#include "bpt.h"
#include <stdbool.h>

#define HEADER(ptr)   ((HeaderPage *)((ptr)->ptr_page))
#define FREE(ptr)     ((FreePage *)((ptr)->ptr_page))
#define LEAF(ptr)     ((LeafPage *)((ptr)->ptr_page))
#define INTERNAL(ptr) ((InternalPage *)((ptr)->ptr_page))


// Size of a page in bytes.
const int PAGE_SIZE = 0x1000;

// Type of the page.
typedef enum {
    HEADER_PAGE,
    FREE_PAGE,
    LEAF_PAGE,
    INTERNAL_PAGE
} PAGE_TYPE;

// Internal/Leaf page have first 120 bytes as a page header.
typedef union {
    char size[120];
    struct {
        off_t parent_page_offset;       // 0, if this page is root.
        int   is_leaf;
        int   number_of_keys;
    };
} PageHeader;

// A record is the pair of key and value.
typedef struct {
    bpt_key_t key;
    char      value[120];
} Record;

// A pair of key and offset.
typedef struct {
    bpt_key_t key;
    off_t     offset;
} KeyOffPair;

// Header page is the first page (offset 0-4095) of a data file, and contains metadata.
typedef union {
    char size[PAGE_SIZE];
    struct {
        off_t     free_page_offset;     // points the first free page (head of free page list)
                                        // 0, if there is no free page left.
        off_t     root_page_offset;
        u_int64_t number_of_pages;
    };
} HeaderPage;

// Free pages are linked and allocation is managed by the free page list.
typedef union {
    char      size[PAGE_SIZE];
    u_int64_t next_free_page_offset;    // points the next free page.
                                        // 0, if end of the free page list.
} FreePage;

// Leaf page contains records(pair of key and value).
typedef struct {
    PageHeader header;
    u_int64_t  right_sibling_page;      // 0, if rightmost leaf page.
    Record     records[31];
} LeafPage;

// Internal page is similar to leaf page, but instead of containing 120 bytes of values,
// it contains 8 bytes of another page (internal or leaf) offset.
// one_more_page < key(0)
// key(0) <= offset(0) < key(1)
// key(1) <= offset(1) < key(2)
// ...
typedef struct {
    PageHeader header;
    off_t      one_more_page;
    KeyOffPair key_offset_pairs[248];
} InternalPage;

// Wrapper struct for single page.
typedef struct {
    void * ptr_page;
    off_t  offset;
} Page;


Page * read_page(off_t offset);
int    write_page(const Page * const page);
void   free_page(Page * page);
Page * get_new_page(PAGE_TYPE type);

#endif