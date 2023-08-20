#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> 
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef struct {
    int cursor;
    int len;
    uint8_t *p;
}Buffer;

Buffer *newBuffer(uint8_t *p, int len);
Buffer *newStack(uint8_t *p, int len);

uint8_t readByte(Buffer *buf);
uint32_t readU32(Buffer *buf);
int32_t readI32(Buffer *buf);
uint32_t readU32_LEB128(Buffer *buf);
int32_t readI32_LEB128(Buffer *buf);
Buffer * readBuffer(Buffer *buf, int len);
char * readName(Buffer *buf);

uint8_t writeByte(Buffer *buf, uint8_t val);
uint32_t writeU32(Buffer *buf, uint32_t val);
int32_t writeI32(Buffer *buf, int32_t val);

bool eof(Buffer *buf);