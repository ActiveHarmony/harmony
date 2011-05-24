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

/* $Revision $ 
   $Date: 2001/12/07 17:12:46 $ */

%{
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "stack.h"
extern int yylex();
extern char* copy(char s);
extern int lineno;

/* extern char *test(char *s);

struct interface *functable;
struct variable *varlist;
struct metric *metriclist;
struct method *methodlist;
struct rule ruletable;
*/
char *buf_s; /* temp string */

//struct typetab *typelist;
%}

%union {

char *string;
struct truthtable *cond;
struct predicate *predi;
struct function *funct;
struct method *meth;
struct metric *metr;
struct convert *conv;

struct interface *inte;
struct variable *vari;

}

%token LIBRARY_SEC INTERFACE_SEC VARIABLE_SEC METRIC_SEC METHOD_SEC RULE_SEC
%token FILE_SEC FUNCTION_SEC ESTIMATION_SEC MEASUREMENT_SEC CONVERTFROM_SEC
%token CONVERTFROM_EST_SEC
%token PREDICATE_SEC TRUTHTABLE_SEC DECISION_SEC

%token CONDITION 

%token MANUAL AUTOMATIC

%token <string> IDENTIFIER  TYPE_NAME 


%type <string> basic_type void_or_int typevar typevars pointers tp opt_init
%type <cond> ttable_def ttables_def
%type <predi> predicate_def predicates_def
%type <funct> metric_implement metrics_implement func_implement funcs_implement
%type <funct> convert_implement converts_implement convert_est_implement convert_ests_implement
%type <meth> lib_def libs_def
%type <metr> metric_def metrics_def id id_list 
%type <conv> conversion_part

%type <inte> function_def functions_def parameter_list parameter 
%type <vari> variables_def variable_def


%token <string> CHAR DOUBLE FLOAT INT LONG SHORT VOID


/*
%token CHAR CONST DOUBLE ENUM EXTERN FLOAT INT LONG REGISTER SHORT SIGNED
%token STATIC STRUCT TYPEDEF UNION UNSIGNED VOID VOLATILE

%token IDENTIFIER 

%token LE_OP GE_OP EQ_OP NE_OP
%token ELLIPSIS

*/

%start program

%%


basic_type:
CHAR  {$$=$1;}| DOUBLE {$$=$1;}| FLOAT {$$=$1;}| INT {$$=$1;}| LONG {$$=$1;}| SHORT {$$=$1;}
;




functions_def:
function_def functions_def { $1->next=$2; $2->prev=$1; $$=$1; }
| function_def { $$=$1; }
;


function_def:
typevars '(' { /* printf("[[%s]]",$1); */ /* (char *)pop());   copy(')'); */ }
parameter_list ')' ';' 
{ $$=(struct interface *)malloc(sizeof(struct interface));
  $$->type=$1; 
  $$->name=(char *)pop(); 
  $$->argu=$4;
  $$->next=$$->prev=NULL;

}
;

typevars:
typevar typevars
{ $$=$1;
  $$=(char *)realloc($$,(strlen($1)+strlen($2)+1)*sizeof(char));
  strcat($$," "); strcat($$,$2);
  free($2);
}
| typevar pointers typevars 
{ $$=$1;
  $$=(char *)realloc($$,(strlen($1)+strlen($2)+strlen($3)+2)*sizeof(char));
  strcat($$," "); strcat($$,$2);
  strcat($$," "); strcat($$,$3);
  free($2); free($3);
}
| typevar
{ push((void *)$1); 
  $$=(char *)strdup("");}
;

typevar:
basic_type
| IDENTIFIER
| VOID
;

pointers:
'*' pointers
{ 
  $$=(char *)strdup("*");

  $$=(char *)realloc($$,(strlen($$)+strlen($2))*sizeof(char));
  strcat($$,$2);
  free($2);

}
| '*'
{ $$=(char *)strdup("*"); }
;


parameter_list:
parameter ',' parameter_list  {$1->next=$3; $3->prev=$1; $$=$1; }
| parameter {$$=$1;}
| {$$=NULL;}
;

parameter:
typevars  
{ $$=(struct interface *)malloc(sizeof(struct interface));
  $$->name=(char *)pop(); 
  $$->type=$1;
}
| tp '(' pointers IDENTIFIER ')' '(' { copy (')'); } ')' 
{ $$=(struct interface *)malloc(sizeof(struct interface));
  $$->name=$4;
  $$->type=$1;
}
;


tp:
typevar tp
{ $$=$1;
  $$=(char *)realloc($$,(strlen($1)+strlen($2)+1)*sizeof(char));
  strcat($$," "); strcat($$,$2);
  free($2);
}
| typevar pointers
{ $$=$1;
  $$=(char *)realloc($$,(strlen($1)+strlen($2)+1)*sizeof(char));
  strcat($$," "); strcat($$,$2);
  free($2); 
}
| typevar
;


/*
types:
typevar pointers
| typevar
;
*/

program:
header_def interface_def var_def mt_def method_def rule_def
;

/* header section */
header_def:
LIBRARY_SEC IDENTIFIER {libname=$2; header=copy('%'); /* headers_list */ }
;

/*
headers_list:
header_list headers_list
| header_list
;

header_list:
TYPE_SEC IDENTIFIER ';' { addtype($2); }
| '#' { copy('\n'); }
| 
;
*/



/* interface section */
interface_def:
INTERFACE_SEC IDENTIFIER functions_def {interfacelang=$2; functable=$3;}
;

/*
functions_def:
function_def functions_def
| function_def
;
*/


/* variable section */
var_def:
VARIABLE_SEC variables_def { varlist=$2; }
;


variables_def:
variable_def   variables_def  
{$1->next=$2; $2->prev=$1; $$=$1;
/* printf("<1<%s,%s>>",$1->type, $1->name); */}
|  variable_def 
{$$=$1;
/* printf("<2<%s,%s>>",$1->type, $1->name); */}
;



variable_def:
tp '@' IDENTIFIER '=' {buf_s=copy(';');} ';'
{ $$=(struct variable *)malloc(sizeof(struct variable));
  $$->type=$1;
  $$->name=$3;
  $$->attr=1;
  $$->init_value=buf_s; 
  $$->next=$$->prev=NULL; 
/* printf("<<%s,%s>>",$$->type, $$->name); */} 
| typevars opt_init ';'
{ $$=(struct variable *)malloc(sizeof(struct variable));
  $$->name=(char *)pop();
  $$->type=$1;
  $$->attr=0;
  $$->init_value=$2;
  $$->next=$$->prev=NULL;
/* printf("<<%s,%s>>",$$->type, $$->name); */
} 
;


opt_init:
'=' { $$=copy(';'); }
| { $$=NULL; } 
;



/* metric section */
mt_def:
METRIC_SEC metrics_def { metriclist=$2; }
;

metrics_def:
metric_def metrics_def  { $1->next=$2; $2->prev=$1; $$=$1; }
| metric_def { $$=$1; }
;

metric_def:
basic_type id_list ';'
{ struct metric *m;
  for (m=$2;m!=NULL;m=m->next)
     m->type=$1;
  $$=$2;
}
;

id_list:
id ',' id_list { $1->next=$3; $3->prev=$1; $$=$1; }
| id {$$=$1;}
;

id:
IDENTIFIER
{ $$=(struct metric *)malloc(sizeof(struct metric));
  $$->name=$1;
  $$->type=NULL;
  $$->next=$$->prev=NULL; 

}
;


/* method section */
method_def:
METHOD_SEC libs_def {methodlist=$2;}
;

libs_def:
lib_def libs_def { $1->next=$2; $2->prev=$1; $$=$1; }
| lib_def { $$=$1; }
;

lib_def:
IDENTIFIER '{' FILE_SEC { push((void *)copy('\n'));/*printf("buf_s=%s\n",buf_s);*/} 
FUNCTION_SEC '{' funcs_implement '}'
ESTIMATION_SEC '{' metrics_implement '}'
MEASUREMENT_SEC '{' metrics_implement '}' 
conversion_part '}'
{ /* printf("buf_s=%s\n",buf_s); */
  $$=(struct method *)malloc(sizeof(struct method));
  $$->name=$1;
  $$->libname=(char *)pop();
  $$->func=$7;
  $$->est=$11;
  $$->mnt=$15;
  $$->cvtfrom=$17->cvtfrom;
  $$->cvtfromest=$17->cvtfromest; /* TODO */
  $$->next=$$->prev=NULL;
  free($17);
}
;

conversion_part:
CONVERTFROM_SEC '{' converts_implement '}'
CONVERTFROM_EST_SEC '{' convert_ests_implement '}' 
{ $$=(struct convert *)malloc(sizeof(struct convert));
  $$->cvtfrom=$3;
  $$->cvtfromest=$7;
}
| CONVERTFROM_SEC '{' converts_implement '}'
{ $$=(struct convert *)malloc(sizeof(struct convert));
  $$->cvtfrom=$3;
  $$->cvtfromest=NULL;
}
| CONVERTFROM_EST_SEC '{' convert_ests_implement '}'
{ $$=(struct convert *)malloc(sizeof(struct convert));
  $$->cvtfrom=NULL;
  $$->cvtfromest=$3;
}
| { $$=(struct convert *)malloc(sizeof(struct convert));
    $$->cvtfrom=NULL;
    $$->cvtfromest=NULL;
  }
;


converts_implement:
convert_implement converts_implement { $1->next=$2; $2->prev=$1; $$=$1; }
| convert_implement {$$=$1;}
;

convert_implement:
IDENTIFIER ':' void_or_int IDENTIFIER '(' {buf_s=copy(')'); } ')' ';' 
{ $$=(struct function *)malloc(sizeof(struct function));
  $$->interface=$1;
  $$->returntype=$3;
  $$->func_call=$4;
  if(buf_s==NULL) $$->argument=(char *)strdup(""); else  $$->argument=buf_s;}
;

convert_ests_implement:
convert_est_implement convert_ests_implement
| convert_est_implement
;

convert_est_implement:
IDENTIFIER ':' IDENTIFIER '(' { buf_s=copy(')'); } ')' ';' 
{ $$=(struct function *)malloc(sizeof(struct function));
  $$->interface=$1;
  $$->returntype=(char *)strdup("void *");
  $$->func_call=$3;
  if(buf_s==NULL) $$->argument=(char *)strdup(""); else  $$->argument=buf_s; }
;

void_or_int:
VOID  {$$=$1;}
| INT {$$=$1;}
| { /* default : void */ $$=(char *)strdup("void"); }
;

funcs_implement:
func_implement funcs_implement { $1->next=$2; $2->prev=$1; $$=$1; }
| func_implement { $$=$1; }
;

func_implement:
IDENTIFIER '(' ')' ':' IDENTIFIER '(' {buf_s=copy(')');} ')' ';'
{ $$=(struct function *)malloc(sizeof(struct function));
  $$->interface=$1;
  $$->func_call=$5;
  if(buf_s==NULL) $$->argument=(char *)strdup(""); else  $$->argument=buf_s;
  $$->returntype=NULL;
  $$->next=$$->prev=NULL; }
;

metrics_implement:
metric_implement metrics_implement { $1->next=$2; $2->prev=$1; $$=$1; }
| metric_implement {$$=$1;}
;

metric_implement:
IDENTIFIER ':' IDENTIFIER '(' { buf_s=copy(')'); } ')' ';'
{ $$=(struct function *)malloc(sizeof(struct function));
  $$->interface=$1;
  $$->func_call=$3;
  if(buf_s==NULL) $$->argument=(char *)strdup(""); else $$->argument=buf_s;
  $$->returntype=NULL;
  $$->next=$$->prev=NULL; }
;

/* rule section */
rule_def:
RULE_SEC PREDICATE_SEC '{' predicates_def '}' TRUTHTABLE_SEC '{' ttables_def '}' decision_part { ruletable.pred=$4; ruletable.table=$8; }
;

predicates_def:
predicate_def predicates_def { $1->next=$2; $2->prev=$1; $$=$1; }
| predicate_def {$$=$1;}
;

predicate_def:
IDENTIFIER ':' { $$=(struct predicate *)malloc(sizeof(struct predicate)); 
$$->name=$1; $$->test=copy('\n'); $$->next=$$->prev=NULL; }
;


ttables_def:
ttable_def ttables_def { $1->next=$2; $2->prev=$1; $$=$1; }
| ttable_def {$$=$1;}
;

ttable_def:
CONDITION { buf_s=copy(':'); } ':' IDENTIFIER { $$=(struct truthtable *)malloc(sizeof(struct truthtable)); $$->target=$4; $$->test=buf_s; $$->next=$$->prev=NULL; /* printf("id:%s",$4); */ }
;


/* TODO */

decision_part:
DECISION_SEC MANUAL { ruletable.decision=(char *)strdup("MANUAL\0"); }
| DECISION_SEC AUTOMATIC {ruletable.decision=(char *)strdup("AUTOMATIC\0"); }
| { ruletable.decision=(char *)strdup("MANUAL\0"); }
;

%%

void init_truthtable(void) {


	InitDQ(ruletable.pred,struct predicate);
	ruletable.pred->name=ruletable.pred->test=NULL;

	//ruletable.no_pred=0;
	InitDQ(ruletable.table,struct truthtable);
	ruletable.table->target=ruletable.table->test=NULL;
	ruletable.decision=NULL;



}

/*
int init_typelist() {

	InitDQ(typelist,struct typetab);
	typelist->name=NULL;

}

struct typetab *addtype(char *s) {

	struct typetab *i;

	for(i=typelist->prev;i!=typelist;i=i->prev)

		if(strcmp(i->name,s)==0)
			break;
	
	if(i!=typelist)
		return i;

	else {
		i=(struct typetab *)malloc(sizeof(struct typetab));
		i->name=(char *)malloc((strlen(s)+1)*sizeof(char));
		strcpy(i->name,s);
		i->name[strlen(s)]='\0';
		i->prev=i->next=NULL;
		InsertDQ(typelist,i);
		return i;
	}
}

*/

void print_rule() {

	struct predicate *p;
	struct truthtable *t;

	printf("predicate:\n");

	for(p=ruletable.pred;p!=NULL;p=p->next)
		if(p->name!=NULL)
			printf("[name]%s [test]%s\n",p->name,p->test);

	printf("truth table:\n");
	
	for(t=ruletable.table;t!=NULL;t=t->next)
		if(t->target!=NULL)
			printf("[test]%s [target]%s\n",t->test,t->target);

	if(ruletable.decision!=NULL)
		printf("[decision]%s\n",ruletable.decision);

}

void print_method () {
		
	struct method *m;
	struct function *f;

	for(m=methodlist;m!=NULL;m=m->next) {

		printf("[name]%s,[libname]%s\n",m->name,m->libname);
		printf("interface\n");
		for(f=m->func;f!=NULL;f=f->next) 
			printf("[interface]%s,[func_call]%s,[argument]%s\n", \
			f->interface,f->func_call,f->argument);
		printf("estimation\n");
                for(f=m->est;f!=NULL;f=f->next) 
                        printf("[interface]%s,[func_call]%s,[argument]%s\n", \
                        f->interface,f->func_call,f->argument);
		printf("measurement\n");
                for(f=m->mnt;f!=NULL;f=f->next) 
                        printf("[interface]%s,[func_call]%s,[argument]%s\n", \
                        f->interface,f->func_call,f->argument);
                printf("convertfrom\n");
                for(f=m->cvtfrom;f!=NULL;f=f->next)
                        printf("[interface]%s,[func_call]%s,[argument]%s\n", \
                        f->interface,f->func_call,f->argument);
                printf("convertfrom_est\n");
                for(f=m->cvtfromest;f!=NULL;f=f->next)
                        printf("[interface]%s,[func_call]%s,[argument]%s\n", \
                        f->interface,f->func_call,f->argument);

	}
}

void print_metric() {

	struct metric *m;
	for(m=metriclist;m!=NULL;m=m->next)
		printf("[type]%s,[name]%s\n",m->type,m->name);

}

/*
void print_type() {

	struct typetab *i;
	for(i=typelist->prev;i!=typelist;i=i->prev)
		printf("[type]%s\n",i->name);
}
*/

void print_functable() {

	struct interface *i,*j;
	for(i=functable;i!=NULL;i=i->next) {
		printf("[type]%s,[name]%s\n",i->type,i->name);
		printf("[argu]\n");
		for(j=i->argu;j!=NULL;j=j->next)
			printf("\t[type]%s,[name]%s\n",j->type,j->name);
	}

}

void print_varlist() {

	struct variable *i;
	for(i=varlist;i!=NULL;i=i->next) {
		printf("[type]%s,[name]%s",i->type,i->name);
		if(i->init_value!=NULL)
			printf(",[value]%s\n",i->init_value);
		else
			printf("\n");

	}

}

int yyerror(char *s) {
   printf ("Error:%s at %d\n", s,lineno);
   return -1;
}

/*
int main(void) {

	stack_init();

	//init_typelist();
	init_truthtable();

	yyparse();

	print_functable();
	print_varlist();

//	print_type();
	print_metric();
	print_method();	
	print_rule();

	return 0;
}
*/

int yywrap()
 { return( 1 ) ; }



