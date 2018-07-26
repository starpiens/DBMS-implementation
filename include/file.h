#ifndef __FILE_H__
#define __FILE_H__

#include <file.h>

// Header page is the first page (offset 0-4095) of a data file, and contains metadata.
typedef union {
    // Fixed size
    char size[PAGE_SIZE];
    // Actual data
    struct {
        // points the first free page (head of free page list)
        // 0, if there is no free page left.
        int64_t free_page_offset;
        // pointing the root page within the data file.
        int64_t root_page_offset;
        // how many pages exist in this data file now.
        int64_t numbef_of_pages;
    };
} HeaderPage;

// Free pages are linked and allocation is managed by the free page list.
typedef union {
    // Fixed size
    char size[PAGE_SIZE];
    // Actual data
    struct {
        // points the next free page. 0, if end of the free page list.
        int64_t next_free_page_offset;
    };
} FreePage;

// Internal/Leaf page have first 128 bytes as a page header.
typedef union {
    // Fixed size
    char size[120];
    // Actual data
    struct {
        // the position of parent page.
        int64_t parent_page_offset;
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
    int64_t right_sibling_page;
    struct {
        int64_t key;
        char value[120];
    } records[31];
} LeafPage;

// Internal page is similar to leaf page, but instead of containing 120 bytes of values,
// it contains 8 bytes of another page (internal or leaf) offset.
typedef struct _InternalPage {
    PageHeader header;
    int64_t one_more_page;
    struct {
        int64_t key;
        int64_t page_offset;
    } key_offset_pairs[248];
} InternalPage;


extern HeaderPage * header_page;


void * read_page(off_t offset);
int write_page(const void * page, off_t offset);


#endif