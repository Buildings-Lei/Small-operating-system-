
#ifndef TASK_H
#define TASK_H

#include "kernel.h"
#include "queue.h"
#include "event.h"
#include "app.h"


typedef struct 
{
    uint gs;
    uint fs;
    uint es;
    uint ds;
    uint edi;
    uint esi;
    uint ebp;
    uint kesp;
    uint ebx;
    uint edx;
    uint ecx;
    uint eax;
    uint error_code; // 错误码
    uint eip;
    uint cs;
    uint eflags;
    uint esp;
    uint ss;
}  RegValue;

//低特权向高特权级转换的时候，需要变换高特权级的栈空间
typedef struct
{
    uint   previous;
    uint   esp0;
    uint   ss0;
    uint   unused[22];
    ushort reserved;
    ushort iomb;
} TSS;

typedef struct
{
    RegValue   rv;         // 现场的上下文的保存。
    Descriptor ldt[3];
    ushort     ldtSelector;
    ushort     tssSelector;
    void (*tmain)();       // 任务需要执行的对象函数。
    uint       id;         // 每一个任务的id唯一，当为 0的时候就是没有任务。
    ushort     current;    // 用于调度时候的优先级计算
    ushort     total;      // 用于优先级计算
    char       name[16];   // 任务的名字
    Queue      wait;       // 加锁失败的任务，解锁的时候需要唤醒他们。
    byte*      stack;      // 私有的栈
    Event*     event;      // 进入等待状态的任务必然指向一个堆中的事件对象
} Task;

typedef struct
{
    QueueNode head;
    Task task;
} TaskNode;

typedef struct
{
    QueueNode head;
    AppInfo app;
} AppNode;

// 事件的状态
enum
{
    WAIT,
    NOTIFY
};

extern void (* const RunTask)(volatile Task* pt);
extern void (* const LoadTask)(volatile Task* pt);

void TaskModInit();
void LaunchTask();
void Schedule();
void TaskCallHandler(uint cmd, uint param1, uint param2);
void EventSchedule(uint action, Event* event);
void KillTask();
void WaitTask(const char* name);

const char* CurrentTaskName();
uint CurrentTaskId();

#endif
