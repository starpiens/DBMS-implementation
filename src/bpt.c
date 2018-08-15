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

/* Find the first key in the 'internal_page' which is greater than 'key'.
 * If success, return the index of upper bound. Otherwise, return -1.
 * When all keys are not greater than 'key', return the last index.
 */
int find_upper_bound_at_internal(Page * internal_page, bpt_key_t key) {
    if (!internal_page || !INTERNAL(internal_page)) return -1;
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
    return -(LEAF(leaf_page)->records[idx].key != key);   // Does lower bound contain 'key'?
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

    // Shift records.
    memcpy(LEAF(leaf_page)->records + idx + sizeof(LEAF(leaf_page)->records[0]), 
           LEAF(leaf_page)->records + idx,
           LEAF(leaf_page)->header.number_of_keys - idx);

    // Insert a new record.
    LEAF(leaf_page)->records[idx].key = key;
    strcpy(LEAF(leaf_page)->records[idx].value, value);

    write_page(leaf_page);
    return 0;
}

/* Insert record into leaf page after splitting.
 * If success, return 0. Otherwise, return -1.
 */
int insert_into_leaf_after_splitting(Page * leaf_page, bpt_key_t key, c_bpt_value_t value) {
    Page * new_leaf = get_tree_page(true);

    return 0;
}

/* Make a new tree.
 * If success, return 0. Otherwise, return -1.
 */
int make_new_tree(bpt_key_t key, c_bpt_value_t value) {
    Page * new_root = get_free_page();
    if (!new_root) return -1;

    LEAF(new_root)->header.parent_page_offset = 0;
    LEAF(new_root)->header.is_leaf            = 1;
    LEAF(new_root)->header.number_of_keys     = 1;
    LEAF(new_root)->right_sibling_page        = 0;
    LEAF(new_root)->records[0].key = key;
    strcpy(LEAF(new_root)->records[0].value, value);
    write_page(new_root);
    
    HEADER(header_page)->root_page_offset = new_root->offset;
    write_page(header_page);
    return 0;
}

/* Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int insert(bpt_key_t key, c_bpt_value_t value) {
    // Case: The tree does not exist yet.
    if (!HEADER(header_page)->root_page_offset) {
        return make_new_tree(key, value);
    }

    Page * leaf_page = find_leaf(key);
    if (!leaf_page) return -1;

    // Case: Leaf page has room for record.
    if (LEAF(leaf_page)->header.number_of_keys < LEAF_ORDER) {
        int ret = insert_into_leaf(leaf_page, key, value);
        free_page(leaf_page);
        return ret;
    }
    
    // Case: Leaf page needs splitting.
    int ret = insert_into_leaf_after_splitting(leaf_page, key, value);
    free_page(leaf_page);
    return ret;
}

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(bpt_key_t key) {
    return 0;
}