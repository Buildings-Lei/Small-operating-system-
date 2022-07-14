#ifndef QUEUE_H
#define QUEUE_H

#include "list.h"

typedef ListNode  QueueNode;

typedef struct {
    ListNode head;
    int length;
} Queue;

void Queue_Init(Queue* queue);
int Queue_IsEmpty(Queue* queue);
int Queue_IsContained(Queue* queue, QueueNode* node);
void Queue_Add(Queue* queue, QueueNode* node);
QueueNode* Queue_Front(Queue* queue);
QueueNode* Queue_Remove(Queue* queue);
int Queue_Length(Queue* queue);
void Queue_Rotate(Queue* queue);

#endif
