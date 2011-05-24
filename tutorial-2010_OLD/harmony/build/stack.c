/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */
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

	
		

		
