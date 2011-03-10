#include <stdlib.h>
#include "queue2.h"
#include "stack.h"


void stack_init() {

	InitDQ(stack,struct stack_ele);
	stack->element=NULL;

}

void push(void *e) {

	struct stack_ele *t=(struct stack_ele *)malloc(sizeof(struct stack_ele));

	t->element=e;
	t->prev=t->next=NULL;

	InsertDQ(stack,t);

}

void *pop(void) {

	struct stack_ele *t=stack->next;
	void *e=t->element;

	if(t==stack) //empty
		return NULL;

	DelDQ(t);
	free(t);

	return e;
}

	
		

		
