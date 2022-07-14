
#ifndef MEMORY_H
#define MEMORY_H

#include "type.h"

void MemModInit(byte* mem, uint size);
void* Malloc(uint size);
void Free(void* ptr);

#endif
