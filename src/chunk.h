// Prevent multiple inclusions of the header file
#ifndef _CHUNK_H_
#define _CHUNK_H_

// Include necessary header files
#include "common.h"
#include "value.h"

// Enum representing bytecode instructions (OpCodes) in the chunk
typedef enum {
    OP_CONSTANT,        // Load a constant value onto the stack
    OP_NIL,             // Push a nil value onto the stack
    OP_TRUE,            // Push a true value onto the stack
    OP_FALSE,           // Push a false value onto the stack
    OP_POP,             // Pop the top value from the stack
    OP_GET_LOCAL,       // Read a local variable's value
    OP_SET_LOCAL,       // Set the value of a local variable
    OP_GET_GLOBAL,      // Read the value of a global variable
    OP_DEFINE_GLOBAL,   // Define a global variable
    OP_SET_GLOBAL,      // Set the value of a global variable
    OP_GET_UPVALUE,     // Read the value of an upvalue
    OP_SET_UPVALUE,     // Set the value of an upvalue
    OP_EQUAL,           // Compare if the top two values on the stack are equal
    OP_GREATER,         // Compare if the second value is greater than the top value
    OP_LESS,            // Compare if the second value is less than the top value
    OP_ADD,             // Add the top two values on the stack
    OP_SUBTRACT,        // Subtract the top value from the second value
    OP_MULTIPLY,        // Multiply the top two values on the stack
    OP_DIVIDE,          // Divide the second value by the top value
    OP_NOT,             // Logical NOT operation on the top value
    OP_NEGATE,          // Negate (unary minus) the top value
    OP_PRINT,           // Print the top value on the stack
    OP_JUMP,            // Unconditional jump to a target position
    OP_JUMP_IF_FALSE,   // Jump if the top value on the stack is false
    OP_LOOP,            // Loop back to a previous position
    OP_CALL,            // Call a function with a specified number of arguments
    OP_CLOSURE,         // Create a closure for a function
    OP_CLOSE_UPVALUE,   // Close an upvalue, marking it as no longer needed
    OP_RETURN,          // Return from the current function
} OpCode;

// Structure representing a chunk of bytecode
typedef struct {
    int count;           // Number of bytecode instructions in the chunk
    int capacity;        // Capacity of the chunk (allocated memory size)
    uint8_t* code;       // Array of bytecode instructions
    int* lines;          // Array of source code line numbers corresponding to each bytecode instruction
    ValueArray constants;// Array of constant values used in the chunk
} Chunk;

// Function prototypes
void initChunk(Chunk* chunk);              // Initialize a chunk
void freeChunk(Chunk* chunk);              // Free resources used by a chunk
void writeChunk(Chunk* chunk, uint8_t byte, int line);  // Write a bytecode instruction to the chunk
int addConstant(Chunk* chunk, Value value); // Add a constant value to the chunk

#endif  // _CHUNK_H_
