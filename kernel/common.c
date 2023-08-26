#include "common.h"

void putchar(char ch);

void *memset(void *buf, char c, size_t n) {
    uint8_t *p = buf;
    while(n--)
        *p++ = c;
    return buf;
}

void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while(n--)
        *d++ = *s++;
    return dst;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while(*src)
        *d++ = *src++;
    *d = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while(*s1 && *s2) {
        if(*s1 != *s2)
            break;
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

void puts(const char *s) {
    while(*s) {
        putchar(*s++);
    }
    putchar('\n');
}

void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case '\0':
                case '%':
                    putchar('%');
                    break;
                case 's': {
                    const char *s = va_arg(vargs, const char *);
                    while (*s) {
                        putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd': {
                    int value = va_arg(vargs, int);
                    if (value < 0) {
                        putchar('-');
                        value = -value;
                    }

                    int divisor = 1;
                    while (value / divisor > 9)
                        divisor *= 10;

                    while (divisor > 0) {
                        putchar('0' + value / divisor);
                        value %= divisor;
                        divisor /= 10;
                    }

                    break;
                }
                case 'x': {
                    int value = va_arg(vargs, int);
                    for (int i = 7; i >= 0; i--) {
                        int nibble = (value >> (i * 4)) & 0xf;
                        putchar("0123456789abcdef"[nibble]);
                    }
                }
            }
        } else {
            putchar(*fmt);
        }

        fmt++;
    }

    va_end(vargs);
}

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
    uint8_t *ptr = next_ptr;

    if(!is_aligned(next_ptr, 0x10))
        next_ptr = align_up(next_ptr, 0x10);

    next_ptr += size;
    
    // PANIC?
    if(next_ptr > __malloc_pool_end)
        return NULL;
    
    memset(ptr, 0, size);
    return ptr;
}