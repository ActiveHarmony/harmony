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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "lsl_config.h"
#include "parser.h"
#include "stack.h"

/* define the file suffix name */
#define chsuffix ".h\0"
#define csuffix ".c\0"
#define cppsuffix ".cpp\0"
#define fortransuffix ".f\0"
#define fortranhsuffix ".F\0"

/* define the template file */
#define templateheaderfile "build/template.h"
#define templatebodyfile "build/template.c"
#define templatefheaderfile "build/template.F"

extern int yyparse();
extern FILE *yyin;

enum _language {_C_LANG,_FORTRAN_LANG,_CPP_LANG};

int methodno,metricno,predicateno,funcno;
int _lang=_C_LANG;    /* 0: c, 1: fortran */

void generate_code();
char *typelookup(char *);

int main(int argc, char **argv) {

	FILE *in;
	int res;

	if (argc!=2) {
		printf("[usage]libcreater src_file\n");
		exit(1);
	}

	if((in=fopen(argv[1],"r"))==NULL) {
		printf("Error opening source file:%s\n",argv[1]);
		exit(1);
	}


	yyin=in;


	/* initial some data structure */
        stack_init();

        //init_typelist();
        init_truthtable();

	/* parser the input file */
        res=yyparse();

	fclose(in);

	if(res==1)  /* parsing error */
		exit(1);

	/* print the datastructure */
/*
	printf("[libname]%s\n[header]%s",libname,header);
        print_functable();
        print_varlist();

        print_type();
        print_metric();
        print_method();
        print_rule();
*/



	/* generate the target code */
	generate_code();

	printf("\n%s%s, %s%s created successfully!\n",libname,chsuffix,libname,csuffix);
	
	if(_lang==_FORTRAN_LANG)
		printf("%s%s created successfully!\n",libname,fortranhsuffix);

	printf("\n");

        return 0;
}

/* transliterate a lower-case string to upper-case */
char *upperstring(char *s) {

	int i;
	char *t;
	t=strdup(s);
	for(i=0;i<strlen(s);i++)
		t[i]=toupper(t[i]);

	return t;
}

/* transliterate a upper-case string to lower-case */
char *lowerstring(char *s) {
	int i;
        char *t;
        t=strdup(s);
        for(i=0;i<strlen(s);i++)
                t[i]=tolower(t[i]);

        return t;
}


/*
	Process the text from FILE *in to FILE *out and	set the constants 
*/
int fcopy(FILE *out, FILE *in, int ns, ...) {

	va_list ap;
	int i,d;
	char c,c1;
	char **ts;

	if(ns<0)
		return -1;

	if(ns>0) { /* assigning the constants to the table from arguments */
		ts=(char **)malloc((ns+1)*sizeof(char *));
		va_start(ap,ns);
		for(i=1;i<=ns;i++) 
			ts[i]=strdup(va_arg(ap,char *));

		va_end(ap);
	}

	while((c=fgetc(in))!=EOF) {

		if(c=='$') {  /* special character */
			if((c1=fgetc(in))=='$') /* if $$ is seen in the template file, stop and return */
				return 1;

			else if ((c1-'0'<0) || (c1-'9'>0)) {

				fprintf(out,"%c%c",c,c1);
				continue;

			}
			else { /* lookup the constants in the table and set it */
				ungetc(c1,in);
				fscanf(in,"%d",&d);
				if((d>ns) || (d<1))
					return -1;
				fprintf(out,"%s",ts[d]);
			}
		}
		else 
			fprintf(out,"%c",c);
	}

	return 0;

}

/* counting the method, metric, predicate and function numbers */
void count_no() {

	struct method *md;
	struct metric *mc;
	struct predicate *pr;
	struct interface *in;

	methodno=0;
	for(md=methodlist;md!=NULL;md=md->next)
		methodno++;

	metricno=0;
	for(mc=metriclist;mc!=NULL;mc=mc->next)
		metricno++;

	predicateno=0;
	for(pr=ruletable.pred;pr!=NULL;pr=pr->next)
		predicateno++;

	funcno=0;
	for(in=functable;in!=NULL;in=in->next)
		funcno++;

	return;
}

/* tranform integer to string */
char *itoa(int d) {

/* TODO: doesn't free the string */

	char *s;
	s=malloc(5*sizeof(char));
	
	sprintf(s,"%d\0",d);

	return s;
}

/* get rid of spaces in the string *s */
char *cleanspace(char *s) {

	int i,j;
	char *t;

	for(i=0;i<strlen(s);i++)
		if(s[i]!=' ')
			break;

	if(i==strlen(s))
		return NULL;

	for(j=strlen(s);j>i;j--)
		if(s[j]!=' ')
			break;

	t=(char *)malloc((j-i+1)*sizeof(char));
	memcpy(t,&(s[i]),(j-i+1)*sizeof(char));

	return t;
}
	

/* generate the target code */
void generate_code() {

	char *fname_h,*fname_b,*fname_fh;
	FILE *in_h,*in_b,*in_fh;
	FILE *out_h,*out_b,*out_fh;
	struct variable *var;
	struct interface *int1,*int2;
	struct method *md;
	struct metric *mc;
	struct predicate *pre;
	struct truthtable *tru;
	struct function *fun;

	char *prednames;
	int first_flag;
	int i;
	char *tmp_str,*tmp_str2;
	char *fullfilename;

	prednames=strdup("");

	/* get statistics */

	count_no();

	/* make function names lower case for fortran */
        /* check the interface language */
	tmp_str=upperstring(interfacelang);
        if(strcmp(tmp_str,"FORTRAN")==0) {
                //printf("fortran detected\n");

		_lang=_FORTRAN_LANG;
                libname=lowerstring(libname);


	}
	free(tmp_str);

	/* create header file */
        fname_h=(char *)malloc((strlen(libname)+strlen(chsuffix))*sizeof(char));
        strcpy(fname_h,libname);
        strcat(fname_h,chsuffix);
        if((out_h=fopen(fname_h,"w"))==NULL) {
                printf("Error opening destination file:%s\n",fname_h);
                exit(1);
        }

	fullfilename=(char *)malloc(sizeof(char)*(strlen(templateheaderfile)+strlen(LSL_PATH)+1));
	sprintf(fullfilename,"%s%s\0",LSL_PATH,templateheaderfile);
        if((in_h=fopen(fullfilename,"r"))==NULL) {
                printf("Error opening template header file:%s\n",templateheaderfile);
                exit(1);
        }
	free(fullfilename);

	/* start header */
	tmp_str=upperstring(libname);
	fcopy(out_h,in_h,1,tmp_str);
	free(tmp_str);

	/*user defined header */
	fprintf(out_h,header);

	/* common header */
	fcopy(out_h,in_h,5,libname,itoa(methodno),itoa(metricno),itoa(predicateno),itoa(funcno));

	/* UpdateAttribute */
		
	fprintf(out_h,"char *updateattribute(");

        first_flag=1;
        for(var=varlist;var!=NULL;var=var->next) {
                if(var->attr==1) {
                        if(first_flag==0)
                                fprintf(out_h,",");
                        fprintf(out_h,"%s _%s",var->type,var->name);
                        first_flag=0;
                }
        }
        fprintf(out_h,");\n");

	/* interface */

	for(int1=functable;int1!=NULL;int1=int1->next) {

	if(_lang==_C_LANG)
		fprintf(out_h,"%s %s(",int1->type,int1->name);
	else if(_lang==_FORTRAN_LANG)
		fprintf(out_h,"%s %s(",int1->type,lowerstring(int1->name));
                for(int2=int1->argu;int2!=NULL;int2=int2->next) {
                        fprintf(out_h,"%s %s",int2->type,int2->name);
			if(int2->next!=NULL)
				fprintf(out_h,",");
		}
		fprintf(out_h,");\n");
        }

        /* common header */
        fcopy(out_h,in_h,0);

	fclose(in_h);
	fclose(out_h);

	/* create body file */
        fname_b=(char *)malloc((strlen(libname)+strlen(csuffix))*sizeof(char));
        strcpy(fname_b,libname);
        strcat(fname_b,csuffix);
        if((out_b=fopen(fname_b,"w"))==NULL) {
                printf("Error opening destination file:%s\n",fname_b);
                exit(1);
        }
        fullfilename=(char *)malloc(sizeof(char)*(strlen(templatebodyfile)+strlen(LSL_PATH)+1));
        sprintf(fullfilename,"%s%s\0",LSL_PATH,templatebodyfile);

        if((in_b=fopen(fullfilename,"r"))==NULL) {
                printf("Error opening template body file:%s\n",templatebodyfile);
                exit(1);
        }
	free(fullfilename);

	/* header in the body */
	fcopy(out_b,in_b,1,libname);

	/* enumeration data type */
	fprintf(out_b,"enum _methodname {");
	for(md=methodlist;md!=NULL;md=md->next) {
		tmp_str=upperstring(md->name);
		fprintf(out_b,"_%s",tmp_str);
		free(tmp_str);
		if(md->next!=NULL)
			fprintf(out_b,",");
	}
	fprintf(out_b,"};\n");
        fprintf(out_b,"enum _metricname {");
        for(mc=metriclist;mc!=NULL;mc=mc->next) {
		tmp_str=upperstring(mc->name);
                fprintf(out_b,"_%s",tmp_str);
		free(tmp_str);
                if(mc->next!=NULL)
                        fprintf(out_b,",");
        }
        fprintf(out_b,"};\n");
        fprintf(out_b,"enum _predicatename {");
        for(pre=ruletable.pred;pre!=NULL;pre=pre->next) {
		tmp_str=upperstring(pre->name);
                fprintf(out_b,"_%s",tmp_str);
		free(tmp_str);
                if(pre->next!=NULL)
                        fprintf(out_b,",");
        }
        fprintf(out_b,"};\n");
        fprintf(out_b,"enum _funcname {");
        for(int1=functable;int1!=NULL;int1=int1->next) {
		tmp_str=upperstring(int1->name);
                fprintf(out_b,"_%s",tmp_str);
		free(tmp_str);
                if(int1->next!=NULL)
                        fprintf(out_b,",");
        }
        fprintf(out_b,"};\n");

	fcopy(out_b,in_b,2,libname,ruletable.decision);

	/* variables */
	for(var=varlist;var!=NULL;var=var->next) {
		fprintf(out_b,"static %s %s",var->type,var->name);
		if(var->init_value!=NULL)
			fprintf(out_b,"=%s",var->init_value);	
		fprintf(out_b,";\n");
	}

	for(pre=ruletable.pred;pre!=NULL;pre=pre->next)
		fprintf(out_b,"static int %s;\n",pre->name);

	/* private functions */
	fcopy(out_b,in_b,2,metriclist->type,libname);

	fprintf(out_b,"void _loadtable() {\n");

        for(pre=ruletable.pred;pre!=NULL;pre=pre->next) {
		tmp_str=malloc((strlen(prednames)+strlen(pre->name)+1)*sizeof(char));
		strcpy(tmp_str,prednames);
		strcat(tmp_str,pre->name);
		free(prednames);
		prednames=tmp_str;

		if(pre->next!=NULL)
			strcat(prednames,",");
                fprintf(out_b,"\tint %s;\n",pre->name);
	}

	for(pre=ruletable.pred;pre!=NULL;pre=pre->next) 
		fprintf(out_b,"\tfor(%s=0;%s<=1;%s++){\n",pre->name,pre->name,pre->name);

	first_flag=1;
	for(tru=ruletable.table;tru!=NULL;tru=tru->next) {

		if(first_flag==0)
			fprintf(out_b,"\telse ");
		else 
			fprintf(out_b,"\t");

		tmp_str=upperstring(tru->target);
		fprintf(out_b,"if %s {\n nb_setdecision(nb_getindex(%s_NOPREDICATE,%s),_%s);\n}\n",tru->test,libname,prednames,tmp_str);
		free(tmp_str);

		first_flag=0;
        }

	for( i=0;i<predicateno;i++)
		fprintf(out_b,"\t}\n");

	fprintf(out_b,"}\n");

	fprintf(out_b,"void _Predicate() {\n");

        for(pre=ruletable.pred;pre!=NULL;pre=pre->next)
                fprintf(out_b,"\t%s=(%s);\n",pre->name,pre->test);

	fprintf(out_b,"\treturn;\n}\n");

	/* init function */

	fprintf(out_b,"void %s_init(void) {\n",libname);

	/* initial method table */
	for(md=methodlist;md!=NULL;md=md->next) {

		tmp_str=upperstring(md->name);
		fprintf(out_b,"\t_methodtable[_%s].name=_setstring(\"%s\");\n", tmp_str,md->name);
		free(tmp_str);
		tmp_str=upperstring(md->name);
		fprintf(out_b,"\t_methodtable[_%s].libfilename=_setstring(\"%s\");\n", tmp_str,cleanspace(md->libname));
		free(tmp_str);
		tmp_str=upperstring(md->name);
		fprintf(out_b,"\t_methodtable[_%s].cvtfunc=(struct %s_cvtfrom*)malloc(%s_NOMETHOD*sizeof(struct %s_cvtfrom));\n",tmp_str,libname,libname,libname);
		free(tmp_str);

		for(fun=md->cvtfrom;fun!=NULL;fun=fun->next) {
		tmp_str=upperstring(md->name);
                tmp_str2=upperstring(fun->interface);
			fprintf(out_b,"\t_methodtable[_%s].cvtfunc[_%s].cvtfuncname=_setstring(\"%s\");\n",tmp_str,tmp_str2,fun->func_call);
		free(tmp_str);
		free(tmp_str2);
		}
		for(fun=md->cvtfromest;fun!=NULL;fun=fun->next) {

                tmp_str=upperstring(md->name);
                tmp_str2=upperstring(fun->interface);

                	fprintf(out_b,"\t_methodtable[_%s].cvtfunc[_%s].cvtestfuncname=_setstring(\"%s\");\n",tmp_str,tmp_str2,fun->func_call);
		free(tmp_str);
		free(tmp_str2);

		}

		tmp_str=upperstring(md->name);

		fprintf(out_b,"\t_methodtable[_%s].mntfunc=(struct %s_mntest*)malloc(%s_NOMETRIC*sizeof(struct %s_mntest));\n",tmp_str,libname,libname,libname);
		free(tmp_str);
	

		for(fun=md->mnt;fun!=NULL;fun=fun->next) {

                tmp_str=upperstring(md->name);
                tmp_str2=upperstring(fun->interface);

			fprintf(out_b,"\t_methodtable[_%s].mntfunc[_%s].mntfuncname=_setstring(\"%s\");\n",tmp_str,tmp_str2,fun->func_call);
		free(tmp_str);
		free(tmp_str2);

		}

                for(fun=md->est;fun!=NULL;fun=fun->next) {

                tmp_str=upperstring(md->name);
                tmp_str2=upperstring(fun->interface);

                        fprintf(out_b,"\t_methodtable[_%s].mntfunc[_%s].estfuncname=_setstring(\"%s\");\n",tmp_str,tmp_str2,fun->func_call);
		free(tmp_str);
		free(tmp_str2);

		}

		tmp_str=upperstring(md->name);

		fprintf(out_b,"\t_methodtable[_%s].interfacefunc=(struct %s_interface *)malloc(%s_NOFUNC*sizeof(struct %s_interface));\n",tmp_str,libname,libname,libname);
		free(tmp_str);

		for(fun=md->func;fun!=NULL;fun=fun->next) {

		tmp_str=upperstring(md->name);
		tmp_str2=upperstring(fun->interface);
                        fprintf(out_b,"\t_methodtable[_%s].interfacefunc[_%s].funcname=_setstring(\"%s\");\n",tmp_str,tmp_str2,fun->func_call);

		free(tmp_str);
		free(tmp_str2);

		}

	}

	/* initial metric table */

	for(mc=metriclist;mc!=NULL;mc=mc->next) {

		tmp_str=upperstring(mc->name);
		fprintf(out_b,"\t_metrictable[_%s].name=_setstring(\"%s\");\n",tmp_str,mc->name);
		free(tmp_str);
		tmp_str=upperstring(mc->name);
		fprintf(out_b,"\t_metrictable[_%s].datatype=_setstring(\"%s\");\n",tmp_str,mc->type);
		free(tmp_str);

	}

	/* set default method and library */

	tmp_str=upperstring(methodlist->name);
	fprintf(out_b,"\t_currentmethodid=_%s;\n",tmp_str);
	free(tmp_str);
	fprintf(out_b,"\t_currentlib=dlopen(_getmethodlib(_currentmethodid),RTLD_LAZY);\n");

	fprintf(out_b,"\t_Predicate();\n");

	/* initial nb */
	fprintf(out_b,"\tnb_init(%s_NOPREDICATE,%s_NOMETHOD,_%scomp,(void *)_%supdateequa);\n",libname,libname,metriclist->type,metriclist->type);

	fprintf(out_b,"\t_loadtable();\n\treturn;\n}\n");

	fcopy(out_b,in_b,1,libname);

	/* UpdateAttribute */

	fprintf(out_b,"char *updateattribute(");

	first_flag=1;
        for(var=varlist;var!=NULL;var=var->next) {
		if(var->attr==1) {
			if(first_flag==0)
				fprintf(out_b,",");
                	fprintf(out_b,"%s _%s",var->type,var->name);
			first_flag=0;
		}
        }
        fprintf(out_b,") {\n");

	fprintf(out_b,"\tchar *suggestname;\n");

	for(var=varlist;var!=NULL;var=var->next) {
                if(var->attr==1) {
			fprintf(out_b,"\t%s=_%s;\n",var->name,var->name);
		}
	}

	fprintf(out_b,"\t_Predicate();\n");

	fprintf(out_b,"\tsuggestname=_getmethodname( nb_suggest(nb_getindex(%s_NOPREDICATE,%s)));\n",libname,prednames);

	fprintf(out_b,"\tif(_decision==_AUTOMATIC)\n\t\tsetmethod(suggestname);\n");
	fprintf(out_b,"\treturn suggestname;\n}\n");

	/* SetMethod */

	fcopy(out_b,in_b,1,libname);

	first_flag=1;
	for(md=methodlist;md!=NULL;md=md->next) {
		for(fun=md->cvtfrom;fun!=NULL;fun=fun->next) {
			if(first_flag==0)
				fprintf(out_b,"\telse ");
			else
				fprintf(out_b,"\t");

	tmp_str=upperstring(md->name);
	tmp_str2=upperstring(fun->interface);
			fprintf(out_b,"if((_currentmethodid==_%s)&&(mt==_%s)) {\n\t\t(*cvtfrom)(%s);\n\t\t_currentmethodid=mt;\n\t\treturn %s_OK;\n\t}\n",tmp_str,tmp_str2,fun->argument,libname);
	
	free(tmp_str);
	free(tmp_str2);

			first_flag=0;
		}
	}

	fcopy(out_b,in_b,1,libname);

	/* Estimate */

	for(mc=metriclist;mc!=NULL;mc=mc->next) {
		fprintf(out_b,"\t%s *_%s=(%s *)(*result);\n",mc->type,mc->name,mc->type);
		fprintf(out_b,"\t%s (*est%s)();\n",mc->type,mc->name);
	}

	fcopy(out_b,in_b,0);
	

	first_flag=1;

	for(md=methodlist;md!=NULL;md=md->next) {

		for(fun=md->est;fun!=NULL;fun=fun->next)  {

		if(first_flag==0)
			fprintf(out_b,"\telse ");
		else	
			fprintf(out_b,"\t");

		tmp_str=upperstring(md->name);
		tmp_str2=upperstring(fun->interface);
		fprintf(out_b,"if((mt==_%s)&&(ms==_%s)) {\n",tmp_str,tmp_str2);

		free(tmp_str);
		free(tmp_str2);

		fprintf(out_b,"\t\test%s=dlsym(lib,_findest(mt,ms));\n",fun->interface);
		fprintf(out_b,"\t\t*_%s=(est%s)(%s);\n\t\tdlclose(lib);\n\t\treturn %s_OK;\n\t}\n",fun->interface,fun->interface,fun->argument,libname);	

		first_flag=0;
		}
	}

	fcopy(out_b,in_b,1,libname);

	/* Estimate Conversion */

	first_flag=1;
	for(md=methodlist;md!=NULL;md=md->next) {

		for(fun=md->cvtfromest;fun!=NULL;fun=fun->next) {

		if(first_flag==0)
			fprintf(out_b,"\telse ");
		else
			fprintf(out_b,"\t");

		tmp_str=upperstring(md->name);
		tmp_str2=upperstring(fun->interface);
		fprintf(out_b,"if((_currentmethodid=_%s)&&(mt==_%s)) {\n",tmp_str,tmp_str2);
		free(tmp_str);
		free(tmp_str2);

		fprintf(out_b,"\t\test=(*cvtfrom)(%s);\n\t\tdlclose(lib);\n\t\treturn %s_OK;\n\t}\n",fun->argument,libname);

		first_flag=0;
		}

	}

	fcopy(out_b,in_b,1,libname);

	/* Measurement */

        for(mc=metriclist;mc!=NULL;mc=mc->next) {
                fprintf(out_b,"\t%s *_%s=(%s *)(*result);\n",mc->type,mc->name,mc->type);
                fprintf(out_b,"\t%s (*mnt%s)();\n",mc->type,mc->name);
        }

	first_flag=1;
	for(md=methodlist;md!=NULL;md=md->next) {
		
		for(fun=md->mnt;fun!=NULL;fun=fun->next) {

		if(first_flag==0)
			fprintf(out_b,"\telse ");
		else
			fprintf(out_b,"\t");

		tmp_str=upperstring(md->name);
		tmp_str2=upperstring(fun->interface);
		fprintf(out_b,"if((_currentmethodid==_%s)&&(ms==_%s)) {\n",tmp_str,tmp_str2);
		free(tmp_str);
		free(tmp_str2);

		fprintf(out_b,"\t\tmnt%s=dlsym(_currentlib,_findmnt(_currentmethodid,ms));\n",fun->interface);

		fprintf(out_b,"\t\t*_%s=(*mnt%s)(%s);\n\t\treturn %s_OK;\n\t}\n",fun->interface,fun->interface,fun->argument,libname);
		first_flag=0;

		}
	}

	fcopy(out_b,in_b,4,libname,metriclist->type,metriclist->name,prednames);

	/* interface */

	for(int1=functable;int1!=NULL;int1=int1->next) {

	if(_lang==_C_LANG)
		fprintf(out_b,"%s %s(",int1->type,int1->name);
	else if(_lang==_FORTRAN_LANG)
		fprintf(out_b,"%s %s(",int1->type,lowerstring(int1->name));

		for(int2=int1->argu;int2!=NULL;int2=int2->next)  {
			fprintf(out_b,"%s %s",int2->type,int2->name);

			if(int2->next!=NULL)
				fprintf(out_b,",");

		}
		fprintf(out_b,") {\n");

		fprintf(out_b,"\t%s (*_%s)();\n",int1->type,int1->name);

		tmp_str=upperstring(int1->name);
		fprintf(out_b,"\t_%s=dlsym(_currentlib,_findfunc(_currentmethodid,_%s));\n",int1->name,tmp_str);
		free(tmp_str);


		first_flag=1;
		for(md=methodlist;md!=NULL;md=md->next) {
			for(fun=md->func;fun!=NULL;fun=fun->next)
				if(strcmp(int1->name,fun->interface)==0) 
					break;

			if(strcmp(int1->name,fun->interface)==0) {

				if(first_flag==0) 
					fprintf(out_b,"\telse ");
				else
					fprintf(out_b,"\t");
				tmp_str=upperstring(md->name);
				fprintf(out_b,"if(_currentmethodid==_%s) {\n",tmp_str);
				free(tmp_str);

				fprintf(out_b,"\t\treturn (*_%s)(%s);\n\t}\n",int1->name,fun->argument);
				first_flag=0;
			}
		}

		fprintf(out_b,"}\n");

	}
			


	fclose(in_b);	
	fclose(out_b);


	/* fortran header file */

	if(_lang==_FORTRAN_LANG) {

        fname_fh=(char *)malloc((strlen(libname)+strlen(fortranhsuffix))*sizeof(char)); 
        strcpy(fname_fh,libname);
        strcat(fname_fh,fortranhsuffix);
        if((out_fh=fopen(fname_fh,"w"))==NULL) {
                printf("Error opening destination file:%s\n",fname_fh);
                exit(1);
	}

        fullfilename=(char *)malloc(sizeof(char)*(strlen(templatefheaderfile)+strlen(LSL_PATH)+1));
        sprintf(fullfilename,"%s%s\0",LSL_PATH,templatefheaderfile);

        if((in_fh=fopen(fullfilename,"r"))==NULL) {
                printf("Error opening template fortran header file:%s\n",templatefheaderfile);
                exit(1);
        }
	free(fullfilename);

	fcopy(out_fh,in_fh,1,libname);

	/* copy the user defined header */
	fprintf(out_fh,header);

	fcopy(out_fh,in_fh,5,libname,itoa(methodno),itoa(metricno),itoa(predicateno),itoa(funcno));

	
        /* interface */

        for(int1=functable;int1!=NULL;int1=int1->next) {

		/* comment the c prototype */

                fprintf(out_fh,"c %s %s(",int1->type,lowerstring(int1->name));


                for(int2=int1->argu;int2!=NULL;int2=int2->next) {
                        fprintf(out_fh,"%s %s",int2->type,int2->name);
                        if(int2->next!=NULL)
                                fprintf(out_fh,",");
                }
                fprintf(out_fh,");\n");


                fprintf(out_fh,"\texternal %s\t!$PRAGMA C(%s)\n",lowerstring(int1->name),lowerstring(int1->name));

		/* get the type */
		if(strstr(int1->type,"*")!=NULL)  /* pointer type */
			fprintf(out_fh,"\tinteger*4 %s\n\n",lowerstring(int1->name));
		else if (strstr(upperstring(int1->type),"VOID")!=NULL) /*void, no return */
			fprintf(out_fh,"\n");
		else
			fprintf(out_fh,"\t%s %s\n\n",typelookup(int1->type),lowerstring(int1->name));
			

        }

	fclose(in_fh);
	fclose(out_fh);
	}

			

}

/* translate appropriate data type from c to fortran */
char *typelookup(char *s) {


	char *t,*r;
	t=upperstring(cleanspace(s));
	
	if(strcmp(t,"INT")==0)
		r=strdup("integer*4");
	else if(strcmp(t,"SHORT")==0)
		r=strdup("integer*2");
	else if(strcmp(t,"FLOAT")==0)
		r=strdup("real*4");
	else if(strcmp(t,"DOUBLE")==0)
		r=strdup("real*8");
	else if(strcmp(t,"CHAR")==0)
		r=strdup("character");
	else if(strcmp(t,"LONG DOUBLE")==0)
		r=strdup("real*16");
	else if(strcmp(t,"LONG LONG INT")==0)
		r=strdup("integer*8");
	else 
		r=strdup("integer*4");

	free(t);

	return r;
}



