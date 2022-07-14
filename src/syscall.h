#ifndef SYSCALL_H
#define SYSCALL_H

#include "type.h"

enum
{
    Normal,
    Strict
};

void Exit();
void Wait(const char* name);
void RegApp(const char* name, void(*tmain)(), byte pri);

uint CreateMutex(uint type);
void EnterCritical(uint mutex);
void ExitCritical(uint mutex);
uint DestroyMutex(uint mutex);

uint ReadKey();
uint GetMemSize();

#endif
