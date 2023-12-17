#ifndef _OBJECT_H_
#define _OBJECT_H_

// Include necessary headers
#include "chunk.h"  // Include the chunk module for handling bytecode chunks
#include "common.h" // Include the common module for shared definitions
#include "helper.h" // Include the helper module for utility functions
#include "value.h"  // Include the value module for value-related definitions

// Object Type Macros

// Get the type of an object from a Value
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

// Check if a value represents a string object
#define IS_STRING(value) isObjType(value, OBJ_STRING)

// Cast a value to a closure object
#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))

// Cast a value to a function object
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

// Extract the native function from a native object
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)

// Cast a value to a string object
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))

// Get the C string from a string object
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

// Object Type Enumeration

typedef enum {
    OBJ_CLOSURE,  // Function closure object
    OBJ_FUNCTION, // Function bytecode object
    OBJ_NATIVE,   // Native function object
    OBJ_STRING,   // String object
    OBJ_UPVALUE   // Upvalue object
} ObjType;

// Object Structure

struct Obj {
    ObjType type;    // Type of the object
    bool isMarked;   // Flag indicating whether the object is marked during garbage collection
    Obj* next;       // Pointer to the next object in the list
};

// Function Object Structure

typedef struct {
    Obj Obj;            // Base object
    int arity;          // Number of parameters expected by the function
    int upvalueCount;   // Number of upvalues used by the function
    Chunk chunk;        // Bytecode chunk representing the function's code
    ObjString* name;    // Optional name associated with the function
} ObjFunction;

// Native Function Object Structure

typedef Value (*NativeFn)(int argCount, Value* args); // Type for native function pointers

typedef struct {
    Obj obj;           // Base object
    NativeFn function; // Pointer to the native function
} ObjNative;

// String Object Structure

struct ObjString {
    Obj obj;            // Base object
    int length;         // Length of the string
    char* chars;        // Array of characters representing the string
    uint32_t hash;      // Hash value for quick string comparison
};

// Upvalue Object Structure

typedef struct ObjUpvalue {
    Obj obj;              // Base object
    Value* location;      // Pointer to the captured variable in the stack frame
    Value closed;         // Value of the captured variable
    struct ObjUpvalue* next; // Pointer to the next upvalue in the linked list
} ObjUpvalue;

// Closure Object Structure

typedef struct {
    Obj obj;              // Base object
    ObjFunction* function;// Pointer to the function associated with the closure
    ObjUpvalue** upvalues; // Array of pointers to upvalues captured by the closure
    int upvalueCount;     // Number of upvalues in the closure
} ObjClosure;

// Function Prototypes

ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);

// Inline function to check if a value has a specific object type
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // _ECLANG_OBJECT_H_