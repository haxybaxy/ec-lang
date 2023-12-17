#ifndef _COMPILER_H_
#define _COMPILER_H_

#include "object.h"
#include "vm.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();

#endif
