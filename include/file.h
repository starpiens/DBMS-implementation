#ifndef __FILE_H__
#define __FILE_H__

#include "sys/types.h"
#include "bpt.h"

// Page size in bytes.
const int PAGE_SIZE = 0x1000;


// Wrapper struct for single page.
typedef struct {
    void * ptr_page;
    off_t offset;
} Page;

// Header page is the first page (offset 0-4095) of a data file, and contains metadata.
typedef union {
    // Fixed size
    char size[PAGE_SIZE];
    // Actual data
    struct {
        // points the first free page (head of free page list)
        // 0, if there is no free page left.
        u_int64_t free_page_offset;
        // pointing the root page within the data file.
        u_int64_t root_page_offset;
        // how many pages exist in this data file now.
        u_int64_t number_of_pages;
    };
} HeaderPage;

// Free pages are linked and allocation is managed by the free page list.
typedef union {
    // Fixed size
    char size[PAGE_SIZE];
    // Actual data
    struct {
        // points the next free page. 0, if end of the free page list.
        u_int64_t next_free_page_offset;
    };
} FreePage;

// Internal/Leaf page have first 128 bytes as a page header.
typedef union {
    // Fixed size
    char size[120];
    // Actual data
    struct {
        // the position of parent page.
        u_int64_t parent_page_offset;
        // 0 is internal page, 1 is leaf page.
        int is_leaf;
        // the number of keys within this page.
        int number_of_keys;
    };
} PageHeader;

// Leaf page contains the key/value records.
typedef struct {
    PageHeader header;
    // If rightmost leaf page, right sibling page offset field is 0.
    u_int64_t right_sibling_page;
    struct {
        bpt_key_t key;
        char value[120];
    } records[31];
} LeafPage;

// Internal page is similar to leaf page, but instead of containing 120 bytes of values,
// it contains 8 bytes of another page (internal or leaf) offset.
typedef struct {
    PageHeader header;
    u_int64_t one_more_page;
    struct {
        bpt_key_t key;
        u_int64_t page_offset;
    } key_offset_pairs[248];
} InternalPage;


Page * read_page(off_t offset);
int write_page(const Page * const page);
int write_page_offset(void * page, off_t offset);

#endif