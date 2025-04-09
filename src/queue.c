#include "queue.h"

#include <stdlib.h>

Queue *Queue_create()
{
    return calloc(1, sizeof(Queue));
}

bool Queue_is_empty(const Queue *queue)
{
    return queue->head == nullptr;
}

void Queue_enqueue(Queue *queue, QueueNode *node)
{
    node->next = nullptr;

    if (!queue->head)
    {
        queue->head = queue->tail = node;
    }
    else
    {
        queue->tail->next = node;
        queue->tail = node;
    }
}

QueueNode *Queue_dequeue(Queue *queue)
{
    if (!queue->head) return nullptr;

    QueueNode *node = queue->head;
    queue->head = queue->head->next;

    if (!queue->head) queue->tail = nullptr;

    return node;
}

void Queue_destroy(Queue *queue)
{
    if (!queue) return;

    while (queue->head)
    {
        QueueNode *node = Queue_dequeue(queue);
        free(node);
    }

    free(queue);
}
