//
// Created by Zaid Saheb on 9/12/23.
//

#ifndef EC_LANG_MEMORY_H
#define EC_LANG_MEMORY_H

#include "common.h"

#define INCREASE_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2) //minimum capacity threshold for the array is 8

#define INCREASE_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif //EC_LANG_MEMORY_H
