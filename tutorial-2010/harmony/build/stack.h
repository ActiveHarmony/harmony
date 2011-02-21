#ifndef _STACK_H
#define _STACK_H


struct stack_ele {

  void *element;
  struct stack_ele *prev;
  struct stack_ele *next;

};

void push(void *);
void *pop(void);

struct stack_ele *stack;


#endif 

