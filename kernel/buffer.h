#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int cursor;
    int len;
    uint8_t *p;
}buffer;

buffer *newbuffer(uint8_t *p, int len);
buffer *newstack(uint8_t *p, int len);

uint8_t readbyte(buffer *buf);
uint32_t readu32(buffer *buf);
int32_t readi32(buffer *buf);
uint32_t readu32_LEB128(buffer *buf);
int32_t readu32_LEB28(buffer *buf);
buffer * readbuffer(buffer *buf, int len);
char * readname(buffer *buf);

uint8_t writebyte(buffer *buf, uint8_t val);
uint32_t writeu32(buffer *buf, uint32_t val);
int32_t writei32(buffer *buf, int32_t val);

uint8_t storebyte(buffer *buf, int32_t offs, uint8_t val);
int32_t storei32(buffer *buf, int32_t offs, int32_t val);

bool eof(buffer *buf);