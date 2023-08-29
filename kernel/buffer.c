#include "buffer.h"
#include "memory.h"

buffer *newbuffer(uint8_t *p, int len) {
    buffer *buf = malloc(sizeof(buffer));

    *buf = (buffer){
        .cursor = 0, 
        .len = len,
        .p = p
    };

    return buf;
}

buffer *newstack(uint8_t *p, int len) {
    buffer *stack = malloc(sizeof(buffer));

    *stack = (buffer){
        .cursor = len, 
        .len = len,
        .p = p
    };

    return stack;
}

uint8_t readbyte(buffer *buf) {
    if(buf->cursor + 1 > buf->len)
        return 0;

    return buf->p[buf->cursor++];
}

uint32_t readu32(buffer *buf) {
    if(buf->cursor + 4 > buf->len)
        return 0;
    
    uint32_t r = *(uint32_t *)&buf->p[buf->cursor];
    buf->cursor += 4;
    return r;
}

int32_t readi32(buffer *buf) {
    if(buf->cursor + 4 > buf->len)
        return 0;
    
    int32_t r = *(int32_t *)&buf->p[buf->cursor];
    buf->cursor += 4;
    return r;
}

// LEB128(Little Endian Base 128)
uint32_t readu32_LEB128(buffer *buf) {
    uint32_t result = 0, shift = 0;
    while(1) {
        uint8_t byte = readbyte(buf); 
        result |= (byte & 0b1111111) << shift;
        shift += 7;
        if((0b10000000 & byte) == 0)
            return result;
    }
}

int32_t readi32_LEB128(buffer *buf) {
    int32_t result = 0, shift = 0;
    while(1) {
        uint8_t byte = readbyte(buf);
        result |= (byte & 0b1111111) << shift;
        shift += 7;
        if((0b10000000 & byte) == 0) {
            if((byte & 0b1000000) != 0)
                return result |= ~0 << shift;
            else
                return result;
        }
    }
}

char * readname(buffer *buf) {
    uint32_t n = readu32_LEB128(buf);
    char *name = malloc(sizeof(char) * (n + 1));

    for(uint32_t i = 0; i < n; i++) {
        name[i] = readbyte(buf);
    }
    name[n] = '\0';
    return name;
}

buffer * readbuffer(buffer *buf, int len) {
    if(buf->cursor + len > buf->len)
        return NULL;

    buffer *new = newbuffer(buf->p + buf->cursor, len);
    buf->cursor += len;
    return new;
}

bool eof(buffer *buf) {
    return buf->cursor == buf->len;
}

// We no longer need to use LSB128 at runtime.
uint8_t wirtebyte(buffer *buf, uint8_t val) {
    if(buf->cursor - 1 < 0)
        return 0;

    buf->p[--buf->cursor] = val;
    return val;
}

uint32_t writeu32(buffer *buf, uint32_t val) {
    if(buf->cursor - 4 < 0)
        return 0;

    buf->cursor -= 4;
    *(uint32_t *)&buf->p[buf->cursor] = val;
    return val;
}

int32_t writei32(buffer *buf, int32_t val) {
    if(buf->cursor - 4 < 0)
        return 0;

    buf->cursor -= 4;
    *(int32_t *)&buf->p[buf->cursor] = val;
    buf->p[buf->cursor] = val;
    return val;
}

uint8_t storebyte(buffer *buf, int32_t offs, uint8_t val) {
    if(offs + 1 > buf->len)
        return 0;

    *(buf->p + offs) = val;
    return val;
}

int32_t storei32(buffer *buf, int32_t offs, int32_t val) {
    if(offs + 4 > buf->len)
        return 0;
    
    *(int32_t *)(buf->p + offs) = val;
    return val;
}