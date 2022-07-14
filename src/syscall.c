
#include "syscall.h"
#include "app.h"

#define SysCall(type, cmd, param1, param2)    asm volatile(                                  \
                                                             "movl  $" #type ",  %%eax \n"   \
                                                             "movl  $" #cmd  ",  %%ebx \n"   \
                                                             "movl  %0,          %%ecx \n"   \
                                                             "movl  %1,          %%edx \n"   \
                                                             "int   $0x80              \n"   \
                                                             :                               \
                                                             : "r"(param1), "r"(param2)      \
                                                             : "eax", "ebx", "ecx", "edx"    \
                                                          )

void Exit()
{
    SysCall(0, 0, 0, 0);
}

void Wait(const char* name)
{
    if( name )
    {
        SysCall(0, 1, name, 0);
    }
}


void RegApp(const char* name, void(*tmain)(), byte pri)
{
    if( name && tmain )
    {
        AppInfo info = {0};
        
        info.name = name;
        info.tmain = tmain;
        info.priority = pri;
        
        SysCall(0, 2, &info, 0);
    }
}


uint CreateMutex(uint type)
{
    volatile uint ret = 0;
    
    SysCall(1, 0, &ret, type); 
    
    return ret;
}

void EnterCritical(uint mutex)
{
    volatile uint wait = 0;
    // 当加锁失败以后 任务睡眠重新执行后，要再一次去抢锁。
    do
    {
        SysCall(1, 1, mutex, &wait);
    }
    while( wait );
}

void ExitCritical(uint mutex)
{
    SysCall(1, 2, mutex, 0);
}

uint DestroyMutex(uint mutex)
{
    uint ret = 0;
    
    SysCall(1, 3, mutex, &ret);
    
    return ret;
}

uint ReadKey()
{
    uint ret = 0;
    
    SysCall(2, 0, &ret, 0);
    
    return ret;
}

uint GetMemSize()
{
    uint ret = 0;
    
    SysCall(3, 0, &ret, 0);
    
    return ret;
}

