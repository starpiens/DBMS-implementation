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

    if ((g_db_file = fopen(pathname, "r+")) == NULL) {
        if ((g_db_file = fopen(pathname, "w+")) == NULL) {
            return 1;
        }
        header_page->free_page_offset = 0x1000;
        header_page->root_page_offset = 0x2000;
        header_page->number_of_pages = 1;
        if (write_page(header_page, 0x0000)) {
            return 1;
        }
    } else if ((header_page = (HeaderPage *)read_page(0)) == NULL) {
        return 1;
    }
    return 0;
}

// Read single page at given offset (of g_db_file).
void * read_page(off_t offset) {
    if (!g_db_file || (offset & 7)) return NULL;
    void * page = calloc(1, PAGE_SIZE);
    if (!page) return NULL;

    fseeko(g_db_file, offset, SEEK_SET);
    if (fread(page, PAGE_SIZE, 1, g_db_file) != PAGE_SIZE) {
        free(page);
        return NULL;
    }
    return page;
}

// Write single page at given offset (of g_db_file).
int write_page(const void * page, off_t offset) {
    if (!g_db_file || (offset & 7)) return 1;
    fseeko(g_db_file, offset, SEEK_SET);
    fwrite(page, PAGE_SIZE, 1, g_db_file);
    fflush(g_db_file);
    return 0;
}

int make_free_pages(u_int64_t num_free_pages) {
    int i;
    for (i = 0; i < num_free_pages; i++) {
        FreePage *new_free_page = (FreePage *)calloc(1, PAGE_SIZE);
        new_free_page->next_free_page_offset = header_page->free_page_offset;
        header_page->free_page_offset = (header_page->number_of_pages + i);
        header_page->number_of_pages++;
        write_page(header_page, 0);
        write_page(new_free_page, (header_page->number_of_pages + i) * PAGE_SIZE);
    }
}