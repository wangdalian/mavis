#include "memory.h"
#include "common.h"

extern uint8_t __pmalloc_pool_start[], __pmalloc_pool_end[];

void *pmalloc(uint32_t n) {
    static uint8_t *next_paddr = __pmalloc_pool_start;
    uint8_t *paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if(next_paddr > __pmalloc_pool_end)
        PANIC("out of memory");

    memset((void *)paddr, 0, n * PAGE_SIZE);
    return paddr;    
}

// todo: impl sbrk and free
extern uint8_t __malloc_pool_start[], __malloc_pool_end[];

void *malloc(size_t size) {
    static uint8_t *next_ptr = __malloc_pool_start;

    next_ptr = align_up(next_ptr, 0x10);

    uint8_t *ptr = next_ptr;

    next_ptr += size;

    if(next_ptr > __malloc_pool_end) {
        PANIC("out of malloc pool");
        return NULL;
    }
    
    memset(ptr, 0, size);
    return ptr;
}