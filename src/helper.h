#ifndef _HELPER_H_
#define _HELPER_H_

// Include necessary headers
#include "common.h" // Include the common module for shared definitions
#include "value.h"  // Include the value module for value-related definitions

// Define data structures

// Entry structure represents a key-value pair in a table
typedef struct {
    ObjString* key; // Key (string) in the entry
    Value value;    // Value associated with the key
} Entry;

// Table structure represents a simple key-value table
typedef struct {
    int count;     // Number of entries in the table
    int capacity;  // Capacity of the table (number of slots)
    Entry* entries; // Array of entries in the table
} Table;

// Declare function prototypes

// Initialize a table instance
void initInstance(Table* table);

// Free resources associated with a table instance
void freeInstance(Table* table);

// Get the value associated with a key in the table
bool getInstance(Table* table, ObjString* key, Value* value);

// Set the value associated with a key in the table
bool setInstance(Table* table, ObjString* key, Value value);

// Delete a key-value pair from the table
bool deleteInstance(Table* table, ObjString* key);

// Find a string in the table based on its characters, length, and hash
ObjString* findStringInstance(Table* table, const char* chars, int length,
                              uint32_t hash);

// Remove white (unmarked) entries from the table during garbage collection
void removeWhiteInstance(Table* table);

// Mark roots relevant to the table during garbage collection
void markInstance(Table* table);


#endif // _HELPER_H_
