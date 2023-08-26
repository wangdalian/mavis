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

// todo: impl sbrk and free
extern uint8_t __malloc_pool_start[], __malloc_pool_end[];

void *malloc(size_t size) {
    static uint8_t *ptr = __malloc_pool_start;

    printf("[+] ptr = 0x%x\n", ptr);

    if(!is_aligned(ptr, 0x10))
        ptr = align_up(ptr, 0x10);
    
    // PANIC?
    if(ptr + size > __malloc_pool_end)
        return NULL;
    
    printf("[+] alloc mem @0x%x, size = %x\n", ptr, size);

    uint8_t *ret = ptr;
    ptr += size;
    return ret;
}