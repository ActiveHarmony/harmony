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

/* $Revision: 1.14 $ $Date: 2001/12/14 02:53:17 $ */
D [0-9]
L [a-zA-Z_]



%{

#include "y.tab.h"
#include "parser.h"

/*
int checktype(char *s);
extern struct typetab *typelist;
*/

int lineno=1;

%}

%%

"/*" {comment();}

"%library"		{ process(); return(LIBRARY_SEC); }
"%interface"		{ process(); return(INTERFACE_SEC); }
"%variable"		{ process(); return(VARIABLE_SEC); }
"%metric"		{ process(); return(METRIC_SEC); }
"%method"		{ process(); return(METHOD_SEC); }
"%rule"			{ process(); return(RULE_SEC); }
"filename"		{ process(); return(FILE_SEC); }
"function"		{ process(); return(FUNCTION_SEC); }
"estimation"		{ process(); return(ESTIMATION_SEC); }
"measurement"		{ process(); return(MEASUREMENT_SEC); }
"convertfrom"		{ process(); return(CONVERTFROM_SEC); }
"convertfrom_est"       { process(); return(CONVERTFROM_EST_SEC); }
"predicate"		{ process(); return(PREDICATE_SEC); }
"truthtable"		{ process(); return(TRUTHTABLE_SEC); }
"decision"		{ process(); return(DECISION_SEC); }
"condition"		{ process(); return(CONDITION); }


"void"			{ process(); yylval.string=(char *)strdup(yytext); return(VOID); }
"int"                   { process(); yylval.string=(char *)strdup(yytext); return(INT); }
"char"			{ process(); yylval.string=(char *)strdup(yytext); return(CHAR);}
"double"		{ process(); yylval.string=(char *)strdup(yytext); return(DOUBLE);}
"float"			{ process(); yylval.string=(char *)strdup(yytext); return(FLOAT);}
"long"			{ process(); yylval.string=(char *)strdup(yytext); return(LONG);}
"short"			{ process(); yylval.string=(char *)strdup(yytext); return(SHORT);}


"manual"		{ process(); return(MANUAL);}
"automatic"		{ process(); return(AUTOMATIC);}

{L}({L}|{D})* { process(); yylval.string=(char *)strdup(yytext); return (IDENTIFIER); /*return (checktype(yytext)); */}

("{"|"<%")              { process(); return('{'); }
("}"|"%>")              { process(); return('}'); }
";"                     { process(); return(';'); }
":"                     { process(); return(':'); }
"("			{ process(); return('('); }
")"			{ process(); return(')'); }
","			{ process(); return(','); }
"@"			{ process(); return('@'); }
"="			{ process(); return('='); }
"#"			{ process(); return('#'); }
"\*"			{ process(); return('*'); }

[ \t\v\f]               { process(); /* ignore spaces */ }
"\n"			{ process(); lineno++; }
.                       { process(); /* ignore bad characters */ }

%%

process() {

//	ECHO;

}

comment()
{
        char c, c1;

/* output start of comment */
while((c = input()) != 0) {
        if(c == '*') {
                if((c1 = input()) == '/' && c1 != 0)

			/* output end of comment */
                        return;
                else
                        unput(c1);
                        /* putchar(c); */
        }
	else if(c == '\n')
		lineno++;
        /*else
                 putchar(c); */

}


}


char * copy(char s)
 {
/* copy all the characters until char s to the output*/

        char c;
        char buf[256];
        char *str=NULL;
        int i=0;

        //printf("(copy:");
        while((c=input())!=s) {

		if(c=='\n') lineno++;

		  
//                if (( c!=' ') && ( c!='\t') && (c!='\n')) {
                        buf[i]=c;
                        i++;
//                }
                        //putchar(c);
        }
        //      ;

        if(i>0) {
                str=(char *)malloc((i+1)*sizeof(char));
                strncpy(str,buf,i);
                str[i]='\0';
        }


        unput(c);
        //printf(")");
        return str;
}
/*
int checkchar(char c) {

	if((c-'a'>=0) && (c-'z'<=0))
		return 1;

	if((c-'A'>=0) && (c-'Z'<=0))
		return 1;

	if((c-'0'>=0) && (c-'9'<=0))
		return 1;

	return 0;

}
*/

/*
char *test(char *s) {

	int i,j;
	int flag=-1;
	int count=0;
	char *t;
	for(i=strlen(s)-1;i>=0;i--) {

		if((flag==-2) && (s[i]==')')) {
			flag=-1;
			continue;
		}
		if((flag==-1) && (s[i]==')')) {
			count++;
			continue;
		}
		if((flag==-1) && (s[i]=='(')) {
			count--;
			if(count==0) 
				flag=0;
			continue;
		}
		if((flag==0)&&(checkchar(s[i])==1)) {
			flag=1;
			continue;

		}
		if((flag==1)&&(checkchar(s[i])==0)) 
			break;
	}
	for(j=strlen(s)-1;j>i;j--) 
		unput(s[j]);

	t=(char *)malloc((i+1)*sizeof(char));

	strncpy(t,s,i+1);

	free(s);

	return t;
}
*/

	

/*

int checktype(char *s) {

	struct typetab *i;

	for(i=typelist->prev;i!=typelist;i=i->prev)

		if(strcmp(i->name,s)==0)
			break;

	if(i!=typelist)
		return TYPE_NAME;

	else
		return IDENTIFIER;
}

*/




