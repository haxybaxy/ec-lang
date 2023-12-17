#ifndef _MEMORY_H_
#define _MEMORY_H_

// Include necessary headers
#include "common.h" // Include the common module for shared definitions
#include "object.h" // Include the object module for object-related definitions

// Memory Management Macros

// Allocate memory for 'count' elements of type 'type'
#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

// Free memory allocated for an object of type 'type'
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// Increase the capacity of a dynamic array
#define INCREASE_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity)*2)

// Increase the size of a dynamic array
#define INCREASE_ARRAY(type, pointer, oldCount, newCount) \
  (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

// Free memory allocated for an array of type 'type'
#define FREE_ARRAY(type, pointer, oldCount) \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

// Declare function prototypes

// Reallocate memory, managing garbage collection and marking
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

// Mark an object as reachable during garbage collection
void markObject(Obj* object);

// Mark a value as reachable during garbage collection
void markValue(Value value);

// Perform garbage collection to free unused memory
void collectGarbage();

// Free all allocated objects during garbage collection
void freeObjects();

#endif // _MEMORY_H_