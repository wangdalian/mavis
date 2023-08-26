#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// clang extention
#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
void puts(const char *s);
void printf(const char *fmt, ...);
void *malloc(size_t size);