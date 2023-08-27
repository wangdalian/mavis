#pragma once

#include <stdint.h> 
#include <stddef.h>
#include "list.h"

#define PAGE_SIZE   4096

struct page {
    list_elem_t link;
    uint8_t     base[];
};

struct malloc_pool {
    list_t  pages;
    uint8_t *next_ptr;
};

void *pmalloc(uint32_t n);
void pfree(void *page);
void *malloc(size_t size);