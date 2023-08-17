#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h>
#include <assert.h>

typedef struct {
    int cursor;
    int len;
    uint8_t *p;
}Buffer;

Buffer *newBuffer(uint8_t *p, int len);
uint8_t readByte(Buffer *buf);
uint32_t readWord(Buffer *buf);
uint32_t readU32(Buffer *buf);
int32_t readI32(Buffer *buf);
bool eof(Buffer *buf);