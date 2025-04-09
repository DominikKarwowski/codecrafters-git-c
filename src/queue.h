#ifndef QUEUE_H
#define QUEUE_H

typedef struct QueueNode
{
    struct QueueNode *next;
    void *value;
} QueueNode;

typedef struct Queue
{
    QueueNode *head;
    QueueNode *tail;
} Queue;

Queue *Queue_create();

bool Queue_is_empty(Queue *queue);

void Queue_enqueue(Queue *queue, QueueNode *node);

QueueNode *Queue_dequeue(Queue *queue);

void Queue_destroy(Queue *queue);

#endif //QUEUE_H
