#include "bpt.h"
#include "file.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define HEADER(ptr)   ((HeaderPage *)((ptr)->ptr_page))
#define FREE(ptr)     ((FreePage *)((ptr)->ptr_page))
#define LEAF(ptr)     ((LeafPage *)((ptr)->ptr_page))
#define INTERNAL(ptr) ((InternalPage *)((ptr)->ptr_page))

// GLOBALS & CONSTANTS.

extern HeaderPage * header_page;

// Order of leaf page.
const int LEAF_ORDER = 32;
// Order of internal page.
const int INTERNAL_ORDER = 249;


// FUNCTION DEFINITIONS.

// Find and return leaf page using binary search.
Page * find_leaf(bpt_key_t key) {
    if (header_page == NULL) return NULL;
    Page * page_ptr = read_page(header_page->root_page_offset);

    while (!INTERNAL(page_ptr)->header.is_leaf) {
        int left = -1, right = INTERNAL(page_ptr)->header.number_of_keys - 2;
        while (left < right) {
            int mid = (left + right) >> 1;
            bpt_key_t mid_key = INTERNAL(page_ptr)->key_offset_pairs[mid + 1].key;
            if (key < mid_key) right = mid;
            else left = mid + 1;
        }

        free(page_ptr);
        u_int64_t next_page_offset = right == -1 ?
            INTERNAL(page_ptr)->one_more_page : INTERNAL(page_ptr)->key_offset_pairs[right].page_offset;
        page_ptr = read_page(next_page_offset);
    }

    return page_ptr;
}

/* Find the first record in the 'leaf_page' which is not less than 'key'.
 * If success, return the index of lower bound. Otherwise, return -1.
 * When all records are less than 'key', return the last index.
 */
int find_lower_bound_at_leaf(Page * leaf_page, bpt_key_t key) {
    if (!leaf_page || !leaf_page->ptr_page) return -1;
    int left = 0, right = LEAF(leaf_page)->header.number_of_keys - 1;
    while (left < right) {
        int mid = (left + right) >> 1;
        bpt_key_t mid_key = LEAF(leaf_page)->records[mid].key;
        
        if (mid_key >= key) right = mid;
        else left = mid + 1;
    }
    return left;
}

/* Find index of key in leaf_page.
 * If found, return index of the key. Otherwise, return -1.
 */
int find_at_leaf(Page * leaf_page, bpt_key_t key) {
    int idx = find_lower_bound_at_leaf(leaf_page, key);
    if (idx < 0 || LEAF(leaf_page)->header.number_of_keys == idx) {
        return -1;
    }
    return -(LEAF(leaf_page)->records[idx].key != key);   // Does lower bound contain 'key'?
}

/* Find the record containing input ‘key’.
 * If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
c_bpt_value_t find(bpt_key_t key) {
    Page * leaf_page = find_leaf(key);
    int idx = find_at_leaf(leaf_page, key);
    bpt_value_t val = idx < 0 ? NULL : LEAF(leaf_page)->records[idx].value;
    free(leaf_page);
    return val;
}

/* Insert record into leaf page.
 * If success, return 0. Otherwise, return -1.
 */
int insert_into_leaf(Page * leaf_page, bpt_key_t key, c_bpt_value_t value) {
    if (LEAF(leaf_page)->header.number_of_keys == LEAF_ORDER) return -1;

    int idx = find_lower_bound_at_leaf(leaf_page, key);
    if (idx < 0) return -1;     // Error
    if (idx != LEAF(leaf_page)->header.number_of_keys && LEAF(leaf_page)->records[idx].key == key) {
        return -1;  // Key duplication
    }

    int shift_sz = sizeof(LEAF(leaf_page)->records[0]);
    memcpy(LEAF(leaf_page)->records + idx + shift_sz, LEAF(leaf_page)->records + idx, LEAF(leaf_page)->header.number_of_keys - idx);
    LEAF(leaf_page)->records[idx].key = key;
    strcpy(LEAF(leaf_page)->records[idx].value, value);

    write_page(leaf_page);
    return 0;
}

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int insert(bpt_key_t key, c_bpt_value_t value) {
    Page * leaf_page = find_leaf(key);

    if (LEAF(leaf_page)->header.number_of_keys < LEAF_ORDER) {
        insert_into_leaf(leaf_page, key, value);
        free(leaf_page);
    }
    
    return 0;
}

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(bpt_key_t key) {
    return 0;
}