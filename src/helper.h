#ifndef _HELPER_H_
#define _HELPER_H_

#include "common.h"
#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initInstance(Table* table);
void freeInstance(Table* table);
bool getInstance(Table* table, ObjString* key, Value* value);
bool setInstance(Table* table, ObjString* key, Value value);
bool deleteInstance(Table* table, ObjString* key);
ObjString* findStringInstance(Table* table, const char* chars, int length,
                              uint32_t hash);
void removeWhiteInstance(Table* table);
void markInstance(Table* table);

#endif
