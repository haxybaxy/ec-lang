#ifndef _COMPILER_H_
#define _COMPILER_H_

// Include necessary headers
#include "object.h" // Include the object module
#include "vm.h"     // Include the virtual machine module

// Declare function prototypes

// Compile source code and return the resulting function
ObjFunction* compile(const char* source);

// Mark roots relevant to the compiler
void markCompilerRoots();

#endif // _COMPILER_H_