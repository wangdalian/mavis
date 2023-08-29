#include "memory.h"
#include "common.h"
#include "list.h"
#include "task.h"

extern uint8_t __pmalloc_pool_start[], __pmalloc_pool_end[];

list_t free_pages = (list_t) {
    .next   = &free_pages,
    .prev   = &free_pages
};

void *pmalloc(uint32_t n) {
    static uint8_t *next_paddr = __pmalloc_pool_start;
    
    // Try to return from the free list first
    struct page *page = LIST_CONTAINER(
        list_tail(&free_pages),
        struct page, 
        link
    );
    list_pop_tail(&free_pages);

    if(page) {
        printf("[+] pmalloc %x\n", page);
        return page;
    }

    // If it fails, cut from the memory pool
    uint8_t *paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if(next_paddr > __pmalloc_pool_end)
        PANIC("out of memory");

    memset((void *)paddr, 0, n * PAGE_SIZE);

    printf("[+] pmalloc %x\n", paddr);

    return paddr;    
}

void pfree(void *page) {
    printf("[-] pfree %x\n", page);
    // The top of the free page is no longer used, so it is used as a header.
    list_push_back(&free_pages, &((struct page *)page)->link);
}

extern struct task *current_task;

void *malloc(size_t size) {
    // get current malloc_pool
    struct malloc_pool *current_pool = &current_task->malloc_pool;

    // get base addr of current page
    struct page *current_page = LIST_CONTAINER(
        list_tail(&current_pool->pages), 
        struct page, 
        link
    );
    uint8_t *base = current_page->base;
    
    uint8_t *next_ptr = current_pool->next_ptr;

    if(next_ptr + size >  base + PAGE_SIZE - sizeof(struct page)) {
        // extend memory
        struct page *new = pmalloc(1);
        
        printf("[+] extend memory: new_page = %x\n", new);

        list_push_back(&current_pool->pages, &new->link);

        // update lcoal variables
        base = new->base;
        next_ptr = align_up(new->base, 0x10);
    }

    uint8_t *ptr = next_ptr;

    // update next_ptr
    current_pool->next_ptr = align_up(next_ptr + size, 0x10);

    printf(
        "[+] alloc mem @0x%x size = %x, next_ptr = %x\n", 
        ptr, 
        size, 
        current_pool->next_ptr
    );

    memset(ptr, 0, size);
    return ptr;
}