#include "utility.h"
#include "task.h"
#include "mutex.h"
#include "queue.h"
#include "app.h"

#define MAX_TASK_NUM        16
#define MAX_RUNNING_TASK    8
#define MAX_READY_TASK      (MAX_TASK_NUM - MAX_RUNNING_TASK)
#define MAX_TASK_BUFF_NUM   (MAX_TASK_NUM + 1)
#define PID_BASE            0x10
#define MAX_TIME_SLICE      260 // 这个保证任务最小的执行时间大于一个时间片，避免一个bug（当都是一个时间片的任务的时候）

void (* const RunTask)(volatile Task* pt) = NULL;
void (* const LoadTask)(volatile Task* pt) = NULL;

volatile Task* gCTaskAddr = NULL; /* DO NOT USE IT DIRECTLY */

// 采用内存池的思想构建task任务，一共可以取17个任务，每一个任务都自己有两个栈空间，
// 内核栈空间为regvalue(保存上下文的作用) ，用户态空间的栈空间为stack，在内存后面中申请的。
static TaskNode gTaskBuff[MAX_TASK_BUFF_NUM] = {0};
static Queue gAppToRun = {0}; // 任务队列的队头

static Queue gFreeTaskNode = {0};// 这个空闲队列节点 利用了池的思想，节点池。
static Queue gReadyTask = {0};
static Queue gRunningTask = {0};
static TSS gTSS = {0};
static TaskNode* gIdleTask = NULL;
static uint gPid = PID_BASE;

// 这个函数作为内核转用户的接口函数，这样任务执行完了，可以对任务池中的任务进行删除。
static void TaskEntry()
{
    if( gCTaskAddr )
    {
        gCTaskAddr->tmain();
    }
    
    // to destory current task here
    asm volatile(
        "movl  $0,  %eax \n"    // type
        "int   $0x80     \n"
    );
}

// 保证至少有一个程序在执行。
static void IdleTask()
{
    while(1);
}

static void InitTask(Task* pt, uint id, const char* name, void(*entry)(), ushort pri)
{
    pt->rv.cs = LDT_CODE32_SELECTOR;
    pt->rv.gs = LDT_VIDEO_SELECTOR;
    pt->rv.ds = LDT_DATA32_SELECTOR;
    pt->rv.es = LDT_DATA32_SELECTOR;
    pt->rv.fs = LDT_DATA32_SELECTOR;
    pt->rv.ss = LDT_DATA32_SELECTOR;
    
    pt->rv.esp = (uint)pt->stack + AppStackSize;
    pt->rv.eip = (uint)TaskEntry;
    pt->rv.eflags = 0x3202;
    
    pt->tmain = entry;
    pt->id = id;
    pt->current = 0;
    pt->total = MAX_TIME_SLICE - pri;
    pt->event = NULL;
    
    if( name )
    {
        StrCpy(pt->name, name, sizeof(pt->name)-1);
    }
    else
    {
        *(pt->name) = 0;
    }
    
    Queue_Init(&pt->wait);
    // 显存段，打印用的
    SetDescValue(AddrOff(pt->ldt, LDT_VIDEO_INDEX),  0xB8000, 0x07FFF, DA_DRWA + DA_32 + DA_DPL3);
    // 局部描述符的代码段，采用段保护保护 核心的堆空间。
    SetDescValue(AddrOff(pt->ldt, LDT_CODE32_INDEX), 0x00,    KernelHeapBase - 1, DA_C + DA_32 + DA_DPL3);
    // 局部描述符的数据段
    SetDescValue(AddrOff(pt->ldt, LDT_DATA32_INDEX), 0x00,    KernelHeapBase - 1, DA_DRW + DA_32 + DA_DPL3);
    
    pt->ldtSelector = GDT_TASK_LDT_SELECTOR;
    pt->tssSelector = GDT_TASK_TSS_SELECTOR;
}

// 找到名字为name 的任务并返回
static Task* FindTaskByName(const char* name)
{
    Task* ret = NULL;
    
    if( !StrCmp(name, "IdleTask", -1) )
    {
        int i = 0;
        
        for(i=0; i<MAX_TASK_BUFF_NUM; i++)
        {
            TaskNode* tn = AddrOff(gTaskBuff, i);
            
            if( tn->task.id && StrCmp(tn->task.name, name, -1) )
            {
                ret = &tn->task;
                break;
            }
        }
    }
    
    return ret;
}

static void PrepareForRun(volatile Task* pt)
{
    pt->current++;
    
    gTSS.ss0 = GDT_DATA32_FLAT_SELECTOR;
    gTSS.esp0 = (uint)&pt->rv + sizeof(pt->rv);
    gTSS.iomb = sizeof(TSS);
    
    SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_LDT_INDEX), (uint)&pt->ldt, sizeof(pt->ldt)-1, DA_LDT + DA_DPL0);
}

static void CreateTask()
{
    while( (0 < Queue_Length(&gAppToRun)) && (Queue_Length(&gReadyTask) < MAX_READY_TASK) )
    {
        TaskNode* tn = (TaskNode*)Queue_Remove(&gFreeTaskNode);
        
        if( tn )
        {
            AppNode* an = (AppNode*)Queue_Remove(&gAppToRun); 
            
            InitTask(&tn->task, gPid++, an->app.name, an->app.tmain, an->app.priority);
            
            Queue_Add(&gReadyTask, (QueueNode*)tn);
            
            Free((void*)an->app.name);
            Free(an);
        }
        else
        {
            break;
        }
    }
}

static void CheckRunningTask()
{
    if( Queue_Length(&gRunningTask) == 0 )
    {
        Queue_Add(&gRunningTask, (QueueNode*)gIdleTask);
    }
    else if( Queue_Length(&gRunningTask) > 1 )
    {
        if( IsEqual(Queue_Front(&gRunningTask), (QueueNode*)gIdleTask) )
        {
            Queue_Remove(&gRunningTask);
        }
    }
}

static void ReadyToRunning()
{
    QueueNode* node = NULL;
    
    if( Queue_Length(&gReadyTask) < MAX_READY_TASK )
    {
        CreateTask();
    }
    
    while( (Queue_Length(&gReadyTask) > 0) && (Queue_Length(&gRunningTask) < MAX_RUNNING_TASK) )
    {
        node = Queue_Remove(&gReadyTask);
        
        ((TaskNode*)node)->task.current = 0;
        
        Queue_Add(&gRunningTask, node);
    }
}

static void RunningToReady()
{
    if( Queue_Length(&gRunningTask) > 0 )
    {
        TaskNode* tn = (TaskNode*)Queue_Front(&gRunningTask);
        
        if( !IsEqual(tn, (QueueNode*)gIdleTask) )
        {
            if( tn->task.current == tn->task.total )
            {
                Queue_Remove(&gRunningTask);
                Queue_Add(&gReadyTask, (QueueNode*)tn);
            }
        }
    }
}

static void RunningToWaitting(Queue* wq)
{
    if( Queue_Length(&gRunningTask) > 0 )
    {
        TaskNode* tn = (TaskNode*)Queue_Front(&gRunningTask);
        
        if( !IsEqual(tn, (QueueNode*)gIdleTask) )
        {
            Queue_Remove(&gRunningTask);
            Queue_Add(wq, (QueueNode*)tn);
        }
    }
}


static void WaittingToReady(Queue* wq)
{
    while( Queue_Length(wq) > 0 )
    {
        TaskNode* tn = (TaskNode*)Queue_Front(wq);
        
        DestroyEvent(tn->task.event);
        
        tn->task.event = NULL;
        
        Queue_Remove(wq);
        Queue_Add(&gReadyTask, (QueueNode*)tn);
    }
}

static void AppInfoToRun(const char* name, void(*tmain)(), byte pri)
{
    AppNode* an = (AppNode*)Malloc(sizeof(AppNode));
    
    if( an )
    {
        char* s = name ? (char*)Malloc(StrLen(name) + 1) : NULL;
        
        an->app.name = s ? StrCpy(s, name, -1) : NULL;
        an->app.tmain = tmain;
        an->app.priority = pri;
        
        Queue_Add(&gAppToRun, (QueueNode*)an);
    }
}

static void AppMainToRun()
{
    AppInfoToRun("AppMain", (void*)(*((uint*)AppMainEntry)), 200);
}

void TaskModInit()
{
    int i = 0;
    // 分配栈空间
    byte* pStack = (byte*)(AppHeapBase - (AppStackSize * MAX_TASK_BUFF_NUM));
    
    for(i=0; i<MAX_TASK_BUFF_NUM; i++)
    {
        TaskNode* tn = (void*)AddrOff(gTaskBuff, i);
        
        tn->task.stack = (void*)AddrOff(pStack, i * AppStackSize);
    }
    
    gIdleTask = (void*)AddrOff(gTaskBuff, MAX_TASK_NUM);
    
    Queue_Init(&gAppToRun);
    Queue_Init(&gFreeTaskNode);
    Queue_Init(&gRunningTask);
    Queue_Init(&gReadyTask);
    
    for(i=0; i<MAX_TASK_NUM; i++)
    {
        Queue_Add(&gFreeTaskNode, (QueueNode*)AddrOff(gTaskBuff, i));
    }
    
    SetDescValue(AddrOff(gGdtInfo.entry, GDT_TASK_TSS_INDEX), (uint)&gTSS, sizeof(gTSS)-1, DA_386TSS + DA_DPL0);
    
    InitTask(&gIdleTask->task, 0, "IdleTask", IdleTask, 255);
    
    AppMainToRun();
    
    ReadyToRunning();
    
    CheckRunningTask();
}

static void ScheduleNext()
{
    ReadyToRunning();
    
    CheckRunningTask();
    
    Queue_Rotate(&gRunningTask);
    
    gCTaskAddr = &((TaskNode*)Queue_Front(&gRunningTask))->task;
    
    PrepareForRun(gCTaskAddr);
    
    LoadTask(gCTaskAddr);
}

void LaunchTask()
{
    gCTaskAddr = &((TaskNode*)Queue_Front(&gRunningTask))->task;
    
    PrepareForRun(gCTaskAddr);
    
    RunTask(gCTaskAddr);
}

void Schedule()
{
    RunningToReady();
    ScheduleNext();
}
// 把该任务放入 需要等待的事件对应的任务中的等待队列当中。
static void WaitEvent(Queue* wait, Event* event)
{
    // 把当前任务产生的事件放入到当前任务结构体中
    gCTaskAddr->event = event;
    // 把当前任务放入到所要等待的任务中（等待队列中）
    RunningToWaitting(wait);
    
    ScheduleNext();
}
// 某一个任务 要等待另外一个任务执行结束。
// 或者说 某一个任务执行结束了，需要将它的等待队列中的任务放入到就绪队列当中。
static void TaskSchedule(uint action, Event* event)
{
    
    Task* task = (Task*)event->id;
    // 走这个分支的场景： 当任务结束后，这个就会通知其他等待这个任务结束的任务。
    if( action == NOTIFY )
    {
        WaittingToReady(&task->wait);
    }
    else if( action == WAIT )
    {
        WaitEvent(&task->wait, event);
    }
}

static void MutexSchedule(uint action, Event* event)
{
    Mutex* mutex = (Mutex*)event->id;
    
    if( action == NOTIFY )
    {
        WaittingToReady(&mutex->wait);
    }
    else if( action == WAIT )
    {
        WaitEvent(&mutex->wait, event);
    }
}

static void KeySchedule(uint action, Event* event)
{
    Queue* wait = (Queue*)event->id;
    // 把kc 给到所有的等待任务中。
    if( action == NOTIFY )
    {
        uint kc = event->param1;
        ListNode* pos = NULL;
        
        List_ForEach((List*)wait, pos)
        {
            TaskNode* tn = (TaskNode*)pos;
            Event* we = tn->task.event;
            uint* ret = (uint*)we->param1;
            
            *ret = kc; // 之前进入休眠的任务保存的是 syscall调用的 &ret
        }
        
        WaittingToReady(wait);
    }
    else if( action == WAIT )
    {
        WaitEvent(wait, event);
    }
}


void EventSchedule(uint action, Event* event)
{
    switch(event->type)
    {
        case KeyEvent:
            KeySchedule(action, event);
            break;
        case TaskEvent:
            TaskSchedule(action, event);
            break;
        case MutexEvent:
            MutexSchedule(action, event);
            break;
        default:
            break;
    }
}

void KillTask()
{
    QueueNode* node = Queue_Remove(&gRunningTask);
    Task* task = &((TaskNode*)node)->task;
    Event evt = {TaskEvent, (uint)task, 0, 0};
    
    EventSchedule(NOTIFY, &evt);
    
    task->id = 0;
    
    Queue_Add(&gFreeTaskNode, node);
    
    Schedule();
}
// name ： 需要等待先执行的任务的名字
// 当前任务需要等待目标任务。
void WaitTask(const char* name)
{
    // 需要被等待的任务地址。
    Task* task = FindTaskByName(name);
    
    if( task )
    {
        Event* evt = CreateEvent(TaskEvent, (uint)task, 0, 0);
        
        if( evt )
        {
            EventSchedule(WAIT, evt);
        }
    }
}



void TaskCallHandler(uint cmd, uint param1, uint param2)
{
    switch(cmd)
    {
        case 0:
            KillTask();
            break;
        case 1:
            WaitTask((char*)param1);
            break;
        case 2:
            AppInfoToRun(((AppInfo*)param1)->name, ((AppInfo*)param1)->tmain, ((AppInfo*)param1)->priority);
            break;
        default:
            break;
    }
}

const char* CurrentTaskName()
{
    return (const char*)gCTaskAddr->name;
}

uint CurrentTaskId()
{
    return gCTaskAddr->id;
}


