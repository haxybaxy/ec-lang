//
// Created by Zaid Saheb on 9/12/23.
//

#ifndef EC_LANG_CHUNK_H
#define EC_LANG_CHUNK_H

#include "common.h"

typedef enum {
    OP_RETURN,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
} Chunk;

#endif //EC_LANG_CHUNK_H
