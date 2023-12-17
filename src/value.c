#include "value.h"
#include <stdio.h>
#include "memory.h"
#include "object.h"

// Initialize a ValueArray structure
void initValueArray(ValueArray* array) {
    array->count = 0;
    array->capacity = 0;
    array->values = NULL;
}

// Write a new value to the ValueArray
void writeValueArray(ValueArray* array, Value value) {
    // Expand the array if necessary
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = INCREASE_CAPACITY(oldCapacity);
        array->values = INCREASE_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }

    // Store the value and increment count
    array->values[array->count] = value;
    array->count++;
}

// Free resources used by a ValueArray
void freeValueArray(ValueArray* array) {
    // Release the allocated memory
    FREE_ARRAY(Value, array->values, array->capacity);
    // Reinitialize the array to a clean state
    initValueArray(array);
}

// Print a Value based on its type
void printValue(Value value) {
#ifdef NAN_BOXING
    // Handle different types of values for NAN_BOXING enabled
    if (IS_BOOL(value)) {
        printf(AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        printf("nil");
    } else if (IS_NUMBER(value)) {
        printf("%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        printObject(value);
    }
#else
    // Handle different types of values for standard implementation
  switch (value.type) {
    case VAL_BOOL:
      printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL:
      printf("nil");
      break;
    case VAL_NUMBER:
      printf("%g", AS_NUMBER(value));
      break;
    case VAL_OBJ:
      printObject(value);
      break;
  }
#endif
}

// Compare two Value objects for equality
bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
    // NAN_BOXING-specific comparison
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    return a == b;
#else
    // Standard type-based comparison
  if (a.type != b.type) return false;
  switch (a.type) {
    case VAL_BOOL:
      return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:
      return true;
    case VAL_NUMBER:
      return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
      return AS_OBJ(a) == AS_OBJ(b);
    default:
      return false;  // Unreachable in a well-formed program.
  }
#endif
}
