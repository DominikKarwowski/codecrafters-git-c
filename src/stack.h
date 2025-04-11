#ifndef STACK_H
#define STACK_H

typedef struct StackNode
{
    struct StackNode *next;
    void *value;
} StackNode;

typedef struct Stack
{
    StackNode *top;
} Stack;

Stack *Stack_create();

bool Stack_is_empty(Stack *stack);

void Stack_push(Stack *stack, void *value);

void *Stack_pop(Stack *stack);

void Stack_destroy(Stack *stack);

#endif //STACK_H
