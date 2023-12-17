#include "chunk.h"
#include "memory.h"
#include "vm.h"

// Initialize a new Chunk
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);  // Initialize the value array for constants
}

// Free the resources used by a Chunk
void freeChunk(Chunk* chunk) {
    // Release memory allocated to code and lines
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);

    // Clean up the constants value array
    freeValueArray(&chunk->constants);

    // Re-initialize the chunk to a clean state
    initChunk(chunk);
}

// Write a byte of code to the Chunk
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    // Expand the chunk if necessary
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = INCREASE_CAPACITY(oldCapacity);
        chunk->code = INCREASE_ARRAY(uint8_t, chunk->code, oldCapacity, chunk->capacity);
        chunk->lines = INCREASE_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }

    // Store the byte and corresponding line number
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// Add a new constant to the Chunk
int addConstant(Chunk* chunk, Value value) {
    // Temporarily store the value on the stack
    push(value);

    // Write the value to the constants array
    writeValueArray(&chunk->constants, value);
    pop();  // Remove the value from the stack

    // Return the index of the new constant
    return chunk->constants.count - 1;
}
