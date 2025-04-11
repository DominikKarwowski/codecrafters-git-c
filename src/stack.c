#include "stack.h"

#include <stdlib.h>

#include "debug_helpers.h"

Stack *Stack_create()
{
    return calloc(1, sizeof(Stack));
}

bool Stack_is_empty(const Stack *stack)
{
    return stack->top == nullptr;
}

void Stack_push(Stack *stack, void *value)
{
    StackNode *node = calloc(1, sizeof(StackNode));
    validate(node, "Failed to allocate memory for the stack element.");

    node->next = nullptr;
    node->value = value;

    if (!stack->top)
    {
        node->next = nullptr;
        stack->top = node;
    }
    else
    {
        node->next = stack->top;
        stack->top = node;
    }

error:
    return;
}

void *Stack_pop(Stack *stack)
{
    if (!stack->top) return nullptr;

    StackNode *node = stack->top;
    stack->top = stack->top->next;

    void *value = node->value;

    free(node);

    return value;
}

void Stack_destroy(Stack *stack)
{
    if (!stack) return;

    while (stack->top)
    {
        (void)Stack_pop(stack);
    }

    free(stack);
}
