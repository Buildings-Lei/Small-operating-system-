
#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "kernel.h"

extern void (* const EnableTimer)();
extern void (* const SendEOI)(uint port);

void IntModInit();
int SetIntHandler(Gate* pGate, uint ifunc);
int GetIntHandler(Gate* pGate, uint* pIFunc);

#endif
