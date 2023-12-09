//
// Created by Zaid Saheb on 9/12/23.
//

#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk -> count = 0;
    chunk -> capacity = 0;
    chunk-> code = NULL;
};

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk -> capacity < chunk -> count + 1) {
        int oldCapacity = chunk -> capacity;
        chunk -> capacity = INCREASE_CAPACITY(oldCapacity);
        chunk -> code = INCREASE_ARRAY(uint8_t, chunk -> code, oldCapacity, chunk->capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->count++;
};


