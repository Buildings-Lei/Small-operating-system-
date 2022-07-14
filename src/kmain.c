#include "task.h"
#include "interrupt.h"
#include "screen.h"
#include "memory.h"
#include "mutex.h"
#include "keyboard.h"
#include "fs.h"

void KMain()
{
    // AppModInit 为 内核态和用户态分离后的用户态的函数的入口。
    void (*AppModInit)() = (void*)BaseOfApp;
    byte* pn = (byte*)0x475;
    
    PrintString("B.L.OS\n");
    
    PrintString("GDT Entry: ");
    PrintIntHex((uint)gGdtInfo.entry);
    PrintChar('\n');
    
    PrintString("GDT Size: ");
    PrintIntDec((uint)gGdtInfo.size);
    PrintChar('\n');
    
    PrintString("IDT Entry: ");
    PrintIntHex((uint)gIdtInfo.entry);
    PrintChar('\n');
    
    PrintString("IDT Size: ");
    PrintIntDec((uint)gIdtInfo.size);
    PrintChar('\n');
    
    PrintString("Number of Hard Disk: ");
    PrintIntDec(*pn);
    PrintChar('\n');
    
    MemModInit((byte*)KernelHeapBase, HeapSize);
    
    KeyboardModInit();
    
    MutexModInit();
    
    // HDRawModInit();
    
    // PrintIntDec(FSIsFormatted());

    AppModInit();

    TaskModInit();
    
    IntModInit();
    // 修改页表属性，让内核态的代码禁止用户态的程序进行修改
    ConfigPageTable();
    

    //while(1);
    
    LaunchTask();
    
}
