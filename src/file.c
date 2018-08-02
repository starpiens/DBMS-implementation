#include "file.h"
#include <stdio.h>
#include <stdlib.h>


// GLOBALS & CONSTANTS.

// File pointer to database file.
static FILE * g_db_file;

// Header page.
HeaderPage * header_page;


// FUNCTION DEFINITIONS.

/* Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return 0. Otherwise, return non-zero value.
 */
int open_db(const char * pathname) {
    // Clean previous data.
    if (g_db_file) fclose(g_db_file);
    if (header_page) free(header_page);

    // Read existing db file
    if ((g_db_file = fopen(pathname, "r+")) == NULL) {
        // If not, make new.
        if ((g_db_file = fopen(pathname, "w+")) == NULL) {
            return 1;
        }

        // Set header page
        header_page->free_page_offset = 0x1000;
        header_page->root_page_offset = 0x2000;
        header_page->number_of_pages = 1;
        if (write_page_offset(header_page, 0)) {
            return 1;
        }
    } else if ((header_page = read_page(0)) == NULL) {
        return 1;
    }
    return 0;
}

// Read single page at given offset (of g_db_file).
// If success, return pointer to the page. Otherwise, return NULL.
Page * read_page(off_t offset) {
    // Read preparation
    if (!g_db_file) return NULL;
    if ((offset & 7) || fseeko(g_db_file, offset, SEEK_SET)) return NULL;
    
    // Memory allocation
    Page * page;
    page->offset = offset;
    if ((page = calloc(1, sizeof(Page))) == NULL ||
        (page->ptr_page = calloc(1, PAGE_SIZE)) == NULL) return NULL;

    // Read
    if (fread(page->ptr_page, PAGE_SIZE, 1, g_db_file) != PAGE_SIZE) {
        free(page->ptr_page);
        free(page);
        return NULL;
    }

    return page;
}

// Write single page.
// If success, return 0. Otherwise, return 1.
int write_page(const Page * const page) {
    // Write preparation
    if (!g_db_file) return 1;
    if ((page->offset & 7) || fseeko(g_db_file, page->offset, SEEK_SET)) return 1;

    // Write and flush
    if (fwrite(page->ptr_page, PAGE_SIZE, 1, g_db_file) != PAGE_SIZE) {
        return 1;
    }
    fflush(g_db_file);

    return 0;
}

// Write single page at given offset.
// If success, return 0. Otherwise, return 1.
int write_page_offset(void * page, off_t offset) {
    Page arg_page = { page, offset };
    return write_page(&arg_page);
}

// Make given number of new free pages.
// If success, return 0. Otherwise, return 1.
int make_free_pages(u_int64_t num_free_pages) {
    int error = 0;
    // Modify header info.
    off_t prev_free_page_offset = header_page->free_page_offset;
    header_page->free_page_offset = header_page->number_of_pages * PAGE_SIZE;
    header_page->number_of_pages += num_free_pages;

    if (write_page_offset(header_page, 0)) {
        header_page->free_page_offset = prev_free_page_offset;
        header_page->number_of_pages -= num_free_pages;
        return 1;
    }
    
    u_int64_t i = 0;
    for (u_int64_t i = 0; i < num_free_pages; i++) {
        FreePage new_free_page;
        if (i < num_free_pages - 1) {
            new_free_page.next_free_page_offset = header_page->free_page_offset + (i + 1) * PAGE_SIZE;
        } else {
            new_free_page.next_free_page_offset = prev_free_page_offset;
        }
        
        write_page_offset(&new_free_page, header_page->free_page_offset + i * PAGE_SIZE);
    }

    return 0;
}

// Get new free page from free page list.
// If success, return pointer to the page. Otherwise, return NULL.
Page * get_free_page(void) {
    if (header_page->free_page_offset == 0) {
        make_free_pages(10);
    }
    
    Page * new_free_page = read_page(header_page->free_page_offset);
    if (new_free_page == NULL) return NULL;

    header_page->free_page_offset = ((FreePage*)(new_free_page->ptr_page))->next_free_page_offset;
    if (write_page(header_page)) {
        header_page->free_page_offset = new_free_page->offset;
        return NULL;
    }

    return new_free_page;
}