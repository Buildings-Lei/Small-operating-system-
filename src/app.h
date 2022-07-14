
#ifndef APP_H
#define APP_H

#include "type.h"

typedef struct 
{
    const char* name;
    void (*tmain)();
    byte priority;
} AppInfo;

#endif
