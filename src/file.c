#include "file.h"
#include <stdio.h>
#include <stdlib.h>


// GLOBALS & CONSTANTS.

// File pointer to database file.
static FILE * g_db_file;

// Header page.
Page * header_page;


// FUNCTION DEFINITIONS.

/* Open existing data file using ‘pathname’ or create one if not existed.
 * If success, return 0. Otherwise, return -1.
 */
int open_db(const char * pathname) {
    // Clean previous data.
    if (g_db_file) fclose(g_db_file);
    if (header_page) free_page(header_page);

    // Read existing DB file.
    if ((g_db_file = fopen(pathname, "r+"))) {
        header_page = read_page(0);
        if (!header_page) return -1;

    // If not, make new.
    } else {
        g_db_file = fopen(pathname, "w+");
        if (!g_db_file) return -1;

        header_page           = calloc(1, sizeof(Page));
        header_page->ptr_page = calloc(1, sizeof(HeaderPage));
        header_page->offset   = 0;

        HEADER(header_page)->free_page_offset = 0;
        HEADER(header_page)->root_page_offset = 0;
        HEADER(header_page)->number_of_pages  = 1;
        
        if (write_page(header_page)) return -1;
    }

    return 0;
}

// Free memory of page.
void free_page(Page * page) {
    if (!page) return;
    if (page->ptr_page) {
        free(page->ptr_page);
    }
    free(page);
}

// Read single page at given offset (of g_db_file).
// If success, return pointer to the page. Otherwise, return NULL.
Page * read_page(off_t offset) {
    // Read preparation
    if (!g_db_file) return NULL;
    if ((offset & 7) || fseeko(g_db_file, offset, SEEK_SET)) return NULL;
    
    // Memory allocation
    Page * page = calloc(1, sizeof(Page));
    if (!page) return NULL;
    page->ptr_page = calloc(1, PAGE_SIZE);
    if (!page->ptr_page) return NULL;

    page->offset = offset;

    // Read
    if (fread(page->ptr_page, PAGE_SIZE, 1, g_db_file) != PAGE_SIZE) {
        free_page(page);
        return NULL;
    }

    return page;
}

// Write single page.
// If success, return 0. Otherwise, return -1.
int write_page(const Page * const page) {
    // Write preparation
    if (!g_db_file) return -1;
    if ((page->offset & 7) || fseeko(g_db_file, page->offset, SEEK_SET)) return -1;

    // Write and flush
    if (fwrite(page->ptr_page, PAGE_SIZE, 1, g_db_file) != PAGE_SIZE) {
        return -1;
    }
    fflush(g_db_file);

    return 0;
}

// Make given number of new free pages.
// If success, return 0. Otherwise, return -1.
int make_free_pages(int num_free_pages) {
    // Modify header info.
    off_t prev_free_page_offset = HEADER(header_page)->free_page_offset;

    HEADER(header_page)->free_page_offset = HEADER(header_page)->number_of_pages * PAGE_SIZE;
    HEADER(header_page)->number_of_pages += num_free_pages;

    if (write_page(header_page)) {
        HEADER(header_page)->free_page_offset = prev_free_page_offset;
        HEADER(header_page)->number_of_pages -= num_free_pages;
        return -1;
    }
    
    for (int i = 1; i <= num_free_pages; i++) {
        FreePage new_free_page;
        new_free_page.next_free_page_offset =
              i == num_free_pages ?     // is last page?
              (HEADER(header_page)->number_of_pages + i) * PAGE_SIZE :
              prev_free_page_offset ;
        
        Page wrapper = { &new_free_page, (HEADER(header_page)->number_of_pages + i - 1) * PAGE_SIZE };
        write_page(&wrapper);
    }

    return 0;
}

// Get new free page from free page list.
// If success, return pointer to the page. Otherwise, return NULL.
Page * get_free_page(void) {
    if (!HEADER(header_page)->free_page_offset) {
        make_free_pages(10);
    }

    Page * new_free_page = read_page(HEADER(header_page)->free_page_offset);
    if (!new_free_page) return NULL;

    HEADER(header_page)->free_page_offset = FREE(new_free_page)->next_free_page_offset;
    if (write_page(header_page)) {
        HEADER(header_page)->free_page_offset = new_free_page->offset;
        return NULL;
    }

    return new_free_page;
}