#include "bpt.h"
#include "file.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


// GLOBALS & CONSTANTS.

extern HeaderPage * header_page;

// Order of leaf page.
const int LEAF_ORDER = 32;
// Order of internal page.
const int INTERNAL_ORDER = 249;


// FUNCTION DEFINITIONS.

// Find and return leaf page using binary search.
LeafPage * find_leaf(bpt_key_t key) {
    if (header_page == NULL) return NULL;
    InternalPage * page_ptr = (InternalPage *)read_page(header_page->root_page_offset);

    while (!page_ptr->header.is_leaf) {
        int left = -1, right = page_ptr->header.number_of_keys - 2;
        while (left < right) {
            int mid = (left + right) / 2;
            bpt_key_t mid_key = page_ptr->key_offset_pairs[mid + 1].key;
            if (key < mid_key) right = mid;
            else left = mid + 1;
        }

        free(page_ptr);
        u_int64_t next_page_offset = right == -1 ?
            page_ptr->one_more_page : page_ptr->key_offset_pairs[right].page_offset;
        page_ptr = (InternalPage *)read_page(next_page_offset);
    }

    return (LeafPage *)page_ptr;
}

// Find key in leaf_page.
// If found, return index of the key. Otherwise, return -1.
int find_at_leaf(LeafPage * leaf_page, bpt_key_t key) {
    int left = 0, right = leaf_page->header.number_of_keys - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        bpt_key_t mid_key = leaf_page->records[mid].key;

        if      (key < mid_key) right = mid - 1;
        else if (key > mid_key) left = mid + 1;
        else    return mid;
    }
    return -1;
}

/* Find the record containing input ‘key’.
 * If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
bpt_value_t find(bpt_key_t key) {
    LeafPage * leaf_page = find_leaf(key);
    int idx = find_at_leaf(leaf_page, key);
    bpt_value_t val = idx < 0 ? NULL : leaf_page->records[idx].value;
    free(leaf_page);
    return val;
}

void insert_into_leaf(LeafPage * leaf_page, bpt_key_t key, c_bpt_value_t value) {
    ;
}

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int insert(bpt_key_t key, c_bpt_value_t value) {
    LeafPage * leaf_page = find_leaf(key);

    if (leaf_page->header.number_of_keys < LEAF_ORDER) {
        insert_into_leaf(leaf_page, key, value);
    }
    
    return 0;
}

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(bpt_key_t key) {
    return 0;
}