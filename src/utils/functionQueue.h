#ifndef FUNCTION_QUEUE_H
#define FUNCTION_QUEUE_H 

constexpr unsigned int maxFunctions = 50;
struct functionStack {
	void (*arr[maxFunctions])(void);
	unsigned int size;
};

extern void functionStack_call(struct functionStack *);
extern void functionStack_insert(struct functionStack *, void (*f)(void));

#endif
