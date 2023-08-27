#pragma once

#include <stdint.h> 
#include <stddef.h>

#define PAGE_SIZE   4096

void *pmalloc(uint32_t n);
void *malloc(size_t size);