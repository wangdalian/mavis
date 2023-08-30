#pragma once

#include <stdint.h>
#include <stdbool.h>

struct buffer {
    int cursor;
    int len;
    uint8_t *p;
};

struct buffer *newbuffer(uint8_t *p, int len);
struct buffer *newstack(uint8_t *p, int len);

uint8_t readbyte(struct buffer *buf);
uint32_t readu32(struct buffer *buf);
int32_t readi32(struct buffer *buf);
uint32_t readu32_LEB128(struct buffer *buf);
int32_t readi32_LEB128(struct buffer *buf);
struct buffer * readbuffer(struct buffer *buf, int len);
char * readname(struct buffer *buf);

uint8_t writebyte(struct buffer *buf, uint8_t val);
uint32_t writeu32(struct buffer *buf, uint32_t val);
int32_t writei32(struct buffer *buf, int32_t val);

uint8_t storebyte(struct buffer *buf, int32_t ea, uint8_t val);
int32_t storei32(struct buffer *buf, int32_t ea, int32_t val);
uint8_t loadbyte(struct buffer *buf, int32_t ea);
int32_t loadi32(struct buffer *buf, int32_t ea);

bool eof(struct buffer *buf);