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

typedef void (*StackNodeCleaner)(void *);

Stack *Stack_create();

bool Stack_is_empty(const Stack *stack);

void Stack_push(Stack *stack, void *value);

void *Stack_pop(Stack *stack);

void *Stack_peek(const Stack *stack);

void Stack_destroy(Stack *stack, StackNodeCleaner cleaner);

#endif //STACK_H
