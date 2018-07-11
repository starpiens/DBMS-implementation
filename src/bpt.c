#include "bpt.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


// STRUCTS & ENUMS

// Header page is the first page (offset 0-4095) of a data file, and contains metadata.
typedef struct _HeaderPage {
    // points the first free page (head of free page list)
    // 0, if there is no free page left.
    int64_t free_page_offset;
    // pointing the root page within the data file.
    int64_t root_page_offset;
    // how many pages exist in this data file now.
    int64_t numbef_of_pages;
    // (Reserved)
    char padding[4072];
} HeaderPage;

// Free pages are linked and allocation is managed by the free page list.
typedef struct _FreePage {
    // points the next free page. 0, if end of the free page list.
    int64_t next_free_page_offset;
    // (Not used)
    char padding[4092];
} FreePage;

// Internal/Leaf page have first 128 bytes as a page header.
typedef struct _PageHeader {
    // If internal/leaf page, this field points the position of parent page.
    int64_t parent_page_offset;
    // 0 is internal page, 1 is leaf page.
    int is_leaf;
    // the number of keys within this page.
    int number_of_keys;
    // (Not used)
    char padding[104];
    union _offset {
        // Used in leaf page. If rightmost leaf page, right sibling page offset field is 0.
        int64_t right_sibling_page;
        // Used in internal page. 
        int64_t one_more_page;
    } offset;
} PageHeader;

// Leaf page contains the key/value pairs.
typedef struct _Record {
    int64_t key;
    char value[120];
} Record;

// Leaf page contains the key/value records.
typedef struct _LeafPage {
    PageHeader header;
    Record records[31];
} LeafPage;

// Internal page contains the key/offset pairs.
typedef struct _KeyOffsetPair {
    int64_t key;
    int64_t page_offset;
} KeyOffsetPair;

// Internal page is similar to leaf page, but instead of containing 120 bytes of values,
// it contains 8 bytes of another page (internal or leaf) offset.
typedef struct _InternalPage {
    PageHeader header;
    KeyOffsetPair key_offset_pairs[248];
} InternalPage;

// The type of pages.
typedef enum _PAGE_TYPE { e_HeaderPage, e_FreePage, e_LeafPage, e_InternalPage } PAGE_TYPE;


// GLOBALS & CONSTANTS.

// Page size in bytes.
const int PAGE_SIZE = 0x1000;
// Order of leaf page.
const int LEAF_ORDER = 32;
// Order of internal page.
const int INTERNAL_ORDER = 249;

// File pointer to database file.
FILE * g_db_file;

// Header page.
HeaderPage * header_page;


// FUNCTION DEFINITIONS.

void * read_page(int64_t offset) {
    if (!g_db_file || (offset & 7)) return NULL;
    void * page = calloc(1, PAGE_SIZE);
    if (!page) return NULL;

    fseek(g_db_file, (long)offset, SEEK_SET);
    if (fread(page, PAGE_SIZE, 1, g_db_file) != PAGE_SIZE) {
        free(page);
        return NULL;
    }
    return page;
}

int write_page(void * page, int64_t offset) {
    if (!g_db_file || (offset & 7)) return 1;
    fseek(g_db_file, (long)offset, SEEK_SET);
    fwrite(page, PAGE_SIZE, 1, g_db_file);
    fflush(g_db_file);
    return 0;
}

int open_db(const char * pathname) {
    // Clean previous data.
    if (g_db_file) fclose(g_db_file);
    if (header_page) free(header_page);

    if ((g_db_file = fopen(pathname, "r+")) == NULL) {
        if ((g_db_file = fopen(pathname, "w+")) == NULL) {
            return 1;
        }
        header_page->free_page_offset = 0x1000;
        header_page->root_page_offset = 0x2000;
        header_page->numbef_of_pages = 0;
        if (write_page(header_page, 0x0000)) {
            return 1;
        }
    } else if ((header_page = (HeaderPage *)read_page(0)) == NULL) {
        return 1;
    }
    return 0;
}

int insert(int64_t key, const char * value) {
    return 0;
}

// Find and return leaf page using binary search.
LeafPage * find_leaf(int64_t key) {
    if (header_page == NULL) return NULL;
    InternalPage * page_ptr = (InternalPage *)read_page(header_page->root_page_offset);

    while (!page_ptr->header.is_leaf) {
        int left = -1, right = page_ptr->header.number_of_keys - 2;
        while (left < right) {
            int mid = (left + right) / 2;
            int64_t mid_key = page_ptr->key_offset_pairs[mid + 1].key;
            if (key < mid_key) right = mid;
            else left = mid + 1;
        }

        free(page_ptr);
        int64_t next_page_offset = right == -1 ?
            page_ptr->header.offset.one_more_page : page_ptr->key_offset_pairs[right].page_offset;
        page_ptr = (InternalPage *)read_page(next_page_offset);
    }

    return (LeafPage *)page_ptr;
}

char * find(int64_t key) {
    LeafPage * page_ptr = find_leaf(key);
    int left = 0, right = page_ptr->header.number_of_keys - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        int64_t mid_key = page_ptr->records[mid].key;

        if (key < mid_key) right = mid - 1;
        else if (key > mid_key) left = mid + 1;
        else {
            free(page_ptr);
            return page_ptr->records[mid].value;
        }
    }
    free(page_ptr);
    return NULL;
}

int delete(int64_t key) {
    return 0;
}