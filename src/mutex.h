
#ifndef MUTEX_H
#define MUTEX_H

#include "type.h"
#include "queue.h"

enum
{
    Normal,
    Strict
};

typedef struct 
{
    ListNode head;
    Queue wait;     // 如果没有锁成功的话，就会进入等待队列。
    uint type;
    uint lock;
} Mutex;

void MutexModInit();
void MutexCallHandler(uint cmd, uint param1, uint param2);


#endif
