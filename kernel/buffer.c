#include "buffer.h"

Buffer *newBuffer(uint8_t *p, int len) {
    Buffer *buf = malloc(sizeof(Buffer));

    *buf = (Buffer){
        .cursor = 0, 
        .len = len,
        .p = p
    };

    return buf;
}

Buffer *newStack(uint8_t *p, int len) {
    Buffer *stack = malloc(sizeof(Buffer));

    *stack = (Buffer){
        .cursor = len, 
        .len = len,
        .p = p
    };

    return stack;
}

uint8_t readByte(Buffer *buf) {
    if(buf->cursor + 1 > buf->len)
        return 0;

    return buf->p[buf->cursor++];
}

uint32_t readU32(Buffer *buf) {
    if(buf->cursor + 4 > buf->len)
        return 0;
    
    uint32_t r = *(uint32_t *)&buf->p[buf->cursor];
    buf->cursor += 4;
    return r;
}

int32_t readI32(Buffer *buf) {
    if(buf->cursor + 4 > buf->len)
        return 0;
    
    int32_t r = *(int32_t *)&buf->p[buf->cursor];
    buf->cursor += 4;
    return r;
}

// LEB128(Little Endian Base 128)
uint32_t readU32_LEB128(Buffer *buf) {
    uint32_t result = 0, shift = 0;
    while(1) {
        uint8_t byte = readByte(buf); 
        result |= (byte & 0b1111111) << shift;
        shift += 7;
        if((0b10000000 & byte) == 0)
            return result;
    }
}

int32_t readI32_LEB128(Buffer *buf) {
    int32_t result = 0, shift = 0;
    while(1) {
        uint8_t byte = readByte(buf);
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

char * readName(Buffer *buf) {
    uint32_t n = readU32_LEB128(buf);
    char *name = malloc(sizeof(char) * (n + 1));

    for(uint32_t i = 0; i < n; i++) {
        name[i] = readByte(buf);
    }
    name[n] = '\0';
    return name;
}

Buffer * readBuffer(Buffer *buf, int len) {
    if(buf->cursor + len > buf->len)
        return NULL;

    Buffer *new = newBuffer(buf->p + buf->cursor, len);
    buf->cursor += len;
    return new;
}

bool eof(Buffer *buf) {
    return buf->cursor == buf->len;
}

// We no longer need to use LSB128 at runtime.
uint8_t writeByte(Buffer *buf, uint8_t val) {
    if(buf->cursor - 1 < 0)
        return 0;

    buf->p[--buf->cursor] = val;
    return val;
}

uint32_t writeU32(Buffer *buf, uint32_t val) {
    if(buf->cursor - 4 < 0)
        return 0;

    buf->cursor -= 4;
    *(uint32_t *)&buf->p[buf->cursor] = val;
    return val;
}

int32_t writeI32(Buffer *buf, int32_t val) {
    if(buf->cursor - 4 < 0)
        return 0;

    buf->cursor -= 4;
    *(int32_t *)&buf->p[buf->cursor] = val;
    buf->p[buf->cursor] = val;
    return val;
}

uint8_t storeByte(Buffer *buf, int32_t offs, uint8_t val) {
    if(offs + 1 > buf->len)
        return 0;

    *(buf->p + offs) = val;
    return val;
}

int32_t storeI32(Buffer *buf, int32_t offs, int32_t val) {
    if(offs + 4 > buf->len)
        return 0;
    
    *(int32_t *)(buf->p + offs) = val;
    return val;
}