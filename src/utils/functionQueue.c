#include "functionQueue.h"

void functionStack_call(struct functionStack *stack) {
	unsigned int n = stack->size;
	for (unsigned int i = n-1; i < maxFunctions; i--)
		stack->arr[i]();
	stack->size = 0;
}

void functionStack_insert(struct functionStack *stack, void (*f)(void)) {
	if (stack->size == maxFunctions) return;
	stack -> arr[stack->size++] = f;
}
