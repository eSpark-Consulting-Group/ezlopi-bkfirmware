#include "bt_event_stack.h"
#include "tal_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Function to create a stack
Stack* createStack(int maxSize, size_t dataSize) {
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    if (!stack) {
        TAL_PR_DEBUG("Memory allocation failed for stack\n");
        return NULL;
    }
    stack->data = malloc(maxSize * dataSize);
    if (!stack->data) {
        TAL_PR_DEBUG("Memory allocation failed for stack data\n");
        free(stack);
        return NULL;
    }
    stack->top = -1;
    stack->maxSize = maxSize;
    stack->dataSize = dataSize;
    return stack;
}

// Function to destroy a stack
void destroyStack(Stack* stack) {
    if (stack) {
        free(stack->data);
        free(stack);
    }
}

// Function to check if the stack is empty
bool isStackEmpty(const Stack* stack) {
    return stack->top == -1;
}

// Function to check if the stack is full
bool isStackFull(const Stack* stack) {
    return stack->top == stack->maxSize - 1;
}

// Function to push an element onto the stack
bool push(Stack* stack, const void* item) {
    if (isStackFull(stack)) {
        TAL_PR_DEBUG("Stack overflow\n");
        return false;
    }
    stack->top++;
    void* target = (char*)stack->data + (stack->top * stack->dataSize);
    memcpy(target, item, stack->dataSize);
    return true;
}

// Function to pop an element from the stack
bool pop(Stack* stack, void* item) {
    if (isStackEmpty(stack)) {
        TAL_PR_DEBUG("Stack underflow\n");
        return false;
    }
    void* source = (char*)stack->data + (stack->top * stack->dataSize);
    memcpy(item, source, stack->dataSize);
    stack->top--;
    return true;
}


// Function to peek at the top element of the stack
bool peek(const Stack* stack, void* item) {
    if (isStackEmpty(stack)) {
        TAL_PR_DEBUG("Stack is empty\n");
        return false;
    }
    void* source = (char*)stack->data + (stack->top * stack->dataSize);
    memcpy(item, source, stack->dataSize);
    return true;
}
   
// Function to print the stack information and elements
void printStack(const Stack* stack, void (*printFunc)(const void*)) {
    if (!stack || isStackEmpty(stack)) {
        TAL_PR_DEBUG("Stack is empty.\n");
        return;
    }

    TAL_PR_DEBUG("Stack Information:\n");
    TAL_PR_DEBUG("Max Size: %d\n", stack->maxSize);
    TAL_PR_DEBUG("Current Size: %d\n", stack->top + 1);

    TAL_PR_DEBUG("Stack Elements:\n");
    for (int i = 0; i <= stack->top; i++) {
        void* item = (char*)stack->data + (i * stack->dataSize);
        TAL_PR_DEBUG("Element %d: ", i + 1);
        printFunc(item);
        TAL_PR_DEBUG("\n");
    }
}