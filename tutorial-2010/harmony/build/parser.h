/*
 * $Revision: 1.11 $
 * $Date: 2001/12/02 21:55:13 $
 * $Id: parser.h,v 1.11 2001/12/02 21:55:13 ihchung Exp $
 */

#ifndef _PARSER_H
#define _PARSER_H


#include "queue2.h"

struct interface {
	char *name;
	char *type;
	struct interface *next;
	struct interface *prev;
	struct interface *argu;
};

struct variable {
	char *name;
	char *type;
	char *init_value;
	int attr;    /* 1: attribute needs update; 0: otherwise */
	struct variable *next;
	struct variable *prev;
};
	


struct metric {

	char *name;
	char *type;
	struct metric *next;
	struct metric *prev;
};

struct function {
	char *interface;
	char *func_call;
	char *argument;
	char *returntype;

	struct function *next;
	struct function *prev;
};

struct convert {
	struct function *cvtfrom;
	struct function *cvtfromest;
};



struct method {

	char *name;
	char *libname;
	struct function *func;
	struct function *est;
	struct function *mnt;
	struct function *cvtfrom;
	struct function *cvtfromest;

	struct method *next;
	struct method *prev;

};

struct predicate {
	char *name;
	char *test;
	struct predicate *next;
	struct predicate *prev;
};

struct truthtable {
	char *target;
	char *test;
	struct truthtable *next;
	struct truthtable *prev;
};
	

struct rule {

	struct predicate *pred;
	struct truthtable *table;
	char *decision;
};

void print_functable();
void print_varlist();
void print_metric();
void print_method ();
void init_truthtable(void);
void print_rule();

char *libname;
char *header;
char *interfacelang;
struct interface *functable;
struct variable *varlist;
struct metric *metriclist;
struct method *methodlist;
struct rule ruletable;

struct typetab *typelist;

	
#endif
