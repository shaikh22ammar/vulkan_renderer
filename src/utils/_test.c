#include "functionQueue.h"
#include <stdio.h>

void pA() {
	printf("a");
}
void pB() {
	printf("b");
}
void pC() {
	printf("c");
}

int main() {
	struct functionStack fS = {0};
	functionStack_insert(&fS, pA);
	functionStack_insert(&fS, pB);
	functionStack_insert(&fS, pC);
	functionStack_insert(&fS, pA);
	
	functionStack_call(fS);
}
