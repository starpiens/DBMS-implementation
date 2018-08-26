#include "bpt.h"
#include "file.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


// GLOBALS & CONSTANTS.

extern Page * header_page;

// Order of leaf page.
const int LEAF_ORDER = 32;
// Order of internal page.
const int INTERNAL_ORDER = 249;


// FUNCTION DEFINITIONS.

/******************************        ******************************/
/*****************************   FIND   *****************************/
/******************************        ******************************/

/* Find the first key in the 'internal_page' which is greater than 'key'.
 * If success, return the index of upper bound. Otherwise, return -1.
 * When all keys are not greater than 'key', return the last index.
 */
int find_upper_bound_at_internal(Page * internal_page, bpt_key_t key) {
    if (!internal_page) return -1;
    // [left, right)
    int left = 0, right = INTERNAL(internal_page)->header.number_of_keys;
    while (left < right) {
        int mid = (left + right) >> 1;
        bpt_key_t mid_key = INTERNAL(internal_page)->key_offset_pairs[mid].key;

        if (key < mid_key) right = mid;
        else left = mid + 1;
    }
    return left;
}

/* Find and return leaf page for 'key'.
 * If success, return pointer to leaf page. Otherwise, return NULL.
 */
Page * find_leaf(bpt_key_t key) {
    if (!header_page) return NULL;
    Page * page_ptr = read_page(0);
    if (!page_ptr) return NULL;

    while (!INTERNAL(page_ptr)->header.is_leaf) {
        int idx = find_upper_bound_at_internal(page_ptr, key);
        off_t next_page_offset = idx ? 
            INTERNAL(page_ptr)->key_offset_pairs[idx - 1].page_offset : 
            INTERNAL(page_ptr)->one_more_page;
        free_page(page_ptr);
        page_ptr = read_page(next_page_offset);
    }

    return page_ptr;
}

/* Find the first record in the 'leaf_page' which is not less than 'key'.
 * If success, return the index of lower bound. Otherwise, return -1.
 * When all records are less than 'key', return the last index.
 */
int find_lower_bound_at_leaf(Page * leaf_page, bpt_key_t key) {
    if (!leaf_page || !LEAF(leaf_page)) return -1;
    // [left, right)
    int left = 0, right = LEAF(leaf_page)->header.number_of_keys;
    while (left < right) {
        int mid = (left + right) >> 1;
        bpt_key_t mid_key = LEAF(leaf_page)->records[mid].key;
        
        if (key <= mid_key) right = mid;
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
    // Does lower bound contain 'key'?
    return LEAF(leaf_page)->records[idx].key == key ?
           idx : -1;
}

/* Find the record containing input ‘key’.
 * If found matching ‘key’, return matched ‘value’ string. Otherwise, return NULL.
 */
c_bpt_value_t find(bpt_key_t key) {
    Page * leaf_page = find_leaf(key);
    int idx = find_at_leaf(leaf_page, key);
    bpt_value_t val = idx < 0 ? NULL : LEAF(leaf_page)->records[idx].value;
    free_page(leaf_page);
    return val;
}


/******************************        ******************************/
/*****************************  INSERT  *****************************/
/******************************        ******************************/




/******************************        ******************************/
/*****************************  DELETE  *****************************/
/******************************        ******************************/

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(bpt_key_t key) {
    return 0;
}