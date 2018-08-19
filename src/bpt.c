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

/* Make new root(internal node) and insert key into it.
 * If success, return 0. Otherwise, return -1.
 */
int insert_into_new_root(Page * left_child, bpt_key_t key, Page * right_child) {

    return 0;
}

int insert_into_internal(Page * internal_page, bpt_key_t key, off_t offset) {
    if (!internal_page) return -1;
    if (INTERNAL(internal_page)->header.number_of_keys == INTERNAL_ORDER) return -1;
    
    int idx = find_upper_bound_at_internal(internal_page, key);
    if (idx < 0) return -1;
    
    // Shift pairs of key and offset.
    const int SIZE = sizeof(INTERNAL(internal_page)->key_offset_pairs[0]);
    memcpy(&INTERNAL(internal_page)->key_offset_pairs[idx] + SIZE,
           &INTERNAL(internal_page)->key_offset_pairs[idx],
           (INTERNAL(internal_page)->header.number_of_keys) * SIZE);

    // Insert
    INTERNAL(internal_page)->key_offset_pairs[idx].key         = key;
    INTERNAL(internal_page)->key_offset_pairs[idx].page_offset = offset;

    return write_page(internal_page);
}

int insert_into_internal_after_splitting(Page * internal_page, off_t left, bpt_key_t key, off_t right) {
    if (!internal_page || !left || !right) return -1;
    if (INTERNAL(internal_page)->header.number_of_keys < INTERNAL_ORDER) {
        return insert_into_internal(internal_page, key, right);
    }

    Page * old_page = internal_page;
    Page * new_page = get_new_page(INTERNAL_PAGE);
    if (!new_page) return -1;

    int split_idx  = INTERNAL_ORDER >> 1;
    int insert_idx = find_upper_bound_at_internal(internal_page, key);
    if (insert_idx < 0) return -1;
    
    // Range of memcpy
    void * start, * end;
    const int SIZE = sizeof(INTERNAL(internal_page)->key_offset_pairs[0]);

    // New pair need to be inserted in old internal page
    if (insert_idx < split_idx) {
        start = &INTERNAL(old_page)->key_offset_pairs[split_idx - 1];
        end   = &INTERNAL(old_page)->key_offset_pairs[INTERNAL_ORDER];
        memcpy(INTERNAL(new_page)->key_offset_pairs, start, end - start);

        start = &INTERNAL(old_page)->key_offset_pairs[insert_idx];
        end   = &INTERNAL(old_page)->key_offset_pairs[split_idx - 1];
        memcpy(start + SIZE, start, end - start);

        INTERNAL(old_page)->key_offset_pairs[insert_idx].key         = key;
        INTERNAL(old_page)->key_offset_pairs[insert_idx].page_offset = right;

    // New pair need to be inserted in new internal page
    } else {
        start = &INTERNAL(old_page)->key_offset_pairs[split_idx];
        end   = &INTERNAL(old_page)->key_offset_pairs[insert_idx];
        memcpy(INTERNAL(new_page)->key_offset_pairs, start, end - start);

        int tmp = end - start + SIZE;

        start = end;
        end   = &INTERNAL(old_page)->key_offset_pairs[INTERNAL_ORDER];
        memcpy(INTERNAL(new_page)->key_offset_pairs + tmp, start, end - start);

        INTERNAL(new_page)->key_offset_pairs[insert_idx - split_idx].key        = key;
        INTERNAL(new_page)->key_offset_pairs[insert_idx - split_idx].page_offset = right;
    }

    INTERNAL(new_page)->header.parent_page_offset = INTERNAL(old_page)->header.parent_page_offset;
    INTERNAL(old_page)->header.number_of_keys = split_idx;
    INTERNAL(new_page)->header.number_of_keys = INTERNAL_ORDER + 1 - split_idx;
    
    write_page(old_page);
    write_page(new_page);

    return insert_into_parent(old_page, key, new_page);
}


/* Insert key into parent page.
 * If success, return 0. Otherwise, return -1.
 */
int insert_into_parent(Page * left_child, bpt_key_t key, Page * right_child) {
    if (!left_child || !right_child) return -1;

    if (!LEAF(left_child)->header.parent_page_offset) {
        return insert_into_new_root(left_child, key, right_child);
    }

    Page * parent_page = read_page(LEAF(left_child)->header.parent_page_offset);
    if (!parent_page) return -1;

    int ret = INTERNAL(parent_page)->header.number_of_keys < INTERNAL_ORDER ?
        insert_into_internal(parent_page, key, right_child->offset) :
        insert_into_internal_after_splitting(parent_page);

    write_page(parent_page);
    free_page(parent_page);
    return ret;
}

/* Insert record into leaf page.
 * If success, return 0. Otherwise, return -1.
 */
int insert_into_leaf(Page * leaf_page, bpt_key_t key, c_bpt_value_t value) {
    if (LEAF(leaf_page)->header.number_of_keys == LEAF_ORDER) return -1;

    int idx = find_lower_bound_at_leaf(leaf_page, key);
    if (idx < 0) return -1;     // Error
    if (idx != LEAF(leaf_page)->header.number_of_keys && 
            LEAF(leaf_page)->records[idx].key == key) {
        return -1;  // Key duplication
    }

    // Shift records.
    int SIZE = sizeof(LEAF(leaf_page)->records[0]);
    memcpy(&LEAF(leaf_page)->records[idx] + SIZE, 
           &LEAF(leaf_page)->records[idx],
           (LEAF(leaf_page)->header.number_of_keys - idx) * SIZE);

    // Insert a new record.
    LEAF(leaf_page)->records[idx].key = key;
    strcpy(LEAF(leaf_page)->records[idx].value, value);
    LEAF(leaf_page)->header.number_of_keys++;

    return write_page(leaf_page);
}

/* Insert record into leaf page after splitting.
 * If success, return 0. Otherwise, return -1.
 */
int insert_into_leaf_after_splitting(Page * leaf_page, bpt_key_t key, c_bpt_value_t value) {
    if (!leaf_page) return -1;
    if (LEAF(leaf_page)->header.number_of_keys < LEAF_ORDER) {
        return insert_into_leaf(leaf_page, key, value);
    }

    Page * old_leaf = leaf_page;
    Page * new_leaf = get_new_page(LEAF_PAGE);
    if (!new_leaf) return -1;

    int split_idx  = LEAF_ORDER >> 1;
    int insert_idx = find_lower_bound_at_leaf(old_leaf, key);
    if (insert_idx < 0) return -1;  // Error 
    if (insert_idx != LEAF(old_leaf)->header.number_of_keys && 
            LEAF(old_leaf)->records[insert_idx].key == key) {
        return -1;  // Key duplication
    }
    
    // Range of memcpy
    void * start, * end;
    const int SIZE = sizeof(LEAF(leaf_page)->records[0]);

    // New record need to be inserted in old leaf page
    if (insert_idx < split_idx) {
        start = &LEAF(old_leaf)->records[split_idx - 1];
        end   = &LEAF(old_leaf)->records[LEAF_ORDER];
        memcpy(LEAF(new_leaf)->records, start, end - start);
        
        start = &LEAF(old_leaf)->records[insert_idx];
        end   = &LEAF(old_leaf)->records[split_idx - 1];
        memcpy(start + SIZE, start, end - start);

        LEAF(old_leaf)->records[insert_idx].key = key;
        strcpy(LEAF(old_leaf)->records[insert_idx].value, value);

    // New record need to be inserted in new leaf page
    } else {
        start = &LEAF(old_leaf)->records[split_idx];
        end   = &LEAF(old_leaf)->records[insert_idx];
        memcpy(LEAF(new_leaf)->records, start, end - start);

        int tmp = end - start + SIZE;

        start = end;
        end   = &LEAF(old_leaf)->records[LEAF_ORDER];
        memcpy(LEAF(new_leaf)->records + tmp, start, end - start);

        LEAF(new_leaf)->records[insert_idx - split_idx].key = key;
        strcpy(LEAF(new_leaf)->records[insert_idx - split_idx].value, value);
    }
    
    LEAF(new_leaf)->header.parent_page_offset = LEAF(old_leaf)->header.parent_page_offset;
    LEAF(old_leaf)->header.number_of_keys = split_idx;
    LEAF(new_leaf)->header.number_of_keys = LEAF_ORDER + 1 - split_idx;
    LEAF(new_leaf)->right_sibling_page = LEAF(old_leaf)->right_sibling_page;
    LEAF(old_leaf)->right_sibling_page = new_leaf->offset;
    
    write_page(old_leaf);
    write_page(new_leaf);

    return insert_into_parent(old_leaf, LEAF(new_leaf)->records[0].key, new_leaf);
}

/* Make a new tree.
 * If success, return 0. Otherwise, return -1.
 */
int make_new_tree(bpt_key_t key, c_bpt_value_t value) {
    Page * new_root = get_new_page(LEAF_PAGE);
    if (!new_root) return -1;

    LEAF(new_root)->header.parent_page_offset = 0;
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

    int ret = LEAF(leaf_page)->header.number_of_keys < LEAF_ORDER ? // Does leaf page has room for record?
        insert_into_leaf(leaf_page, key, value) :                   // T - Leaf page has room for record.
        insert_into_leaf_after_splitting(leaf_page, key, value);    // F - Leaf page must be splitted.

    free_page(leaf_page);
    return ret;
}

/* Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(bpt_key_t key) {
    return 0;
}