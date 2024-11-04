#ifndef STACK_H
#define STACK_H

#include <stdbool.h>
#include <stddef.h>

// Stack structure
typedef struct Stack {
    void* data;       // Pointer to hold the stack data
    int top;          // Index of the top element
    int maxSize;      // Maximum number of elements
    size_t dataSize;  // Size of each element in bytes
} Stack;

// Stack management functions
Stack* createStack(int maxSize, size_t dataSize);
void destroyStack(Stack* stack);

bool push(Stack* stack, const void* item);
bool pop(Stack* stack, void* item);
bool peek(const Stack* stack, void* item);
bool isStackEmpty(const Stack* stack);
bool isStackFull(const Stack* stack);

void printStack(const Stack* stack, void (*printFunc)(const void*));

#endif // STACK_H
