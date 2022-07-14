
#ifndef EVENT_H
#define EVENT_H

#include "type.h"

enum
{
    NoneEvent,
    MutexEvent,
    KeyEvent,
    TaskEvent
};

typedef struct
{
    uint type; // 描述事件类型（为什么发生状态切换）
    uint id;  //  事件标识信息（调度关键），指的是需要放入的等待队列，如果包装的是任务的话，就是任务中的等待队列。
             //   在键盘中，指的是全局等待队列。
    uint param1;  // 参数 1
    uint param2;  // 参数 2
} Event;

Event* CreateEvent(uint type, uint id, uint param1, uint param2);
void DestroyEvent(Event* event);


#endif
