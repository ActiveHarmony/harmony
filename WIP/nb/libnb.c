/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
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

/* $Revision: 1.5 $ */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "libnb.h"


/* calculate x^y */
int power(int x, int y) {

	if(y==0) 
		return 1;
	return (x*power(x,y-1));


}

/* initizlize the knowledge base */
int nb_init(int no_attr, int no_tgt, int (*comp)(void *,void *), void *(*update_equation)(void *, void *)) {

/*
	The comp(a,b) should return 1 if a is prefered to b.
*/

	int i,j;

	nb_compare=comp;
	no_attribute=no_attr;
	no_tgt_value=no_tgt;
	nb_compare=comp;
	nb_equation=update_equation;

	if(no_attr>sizeof(int)*8-2)
		return NB_INDEX_ERR;

	if(comp==NULL)
		return NB_FUNC_ERR;


	nb_table=(struct nb_table_ele *)malloc(power(2,no_attribute)*sizeof(struct nb_table_ele));

	/* return insufficient memory error here */
	if(nb_table==NULL)
		return NB_INIT_ERR;

	for(i=0;i<power(2,no_attribute);i++) {

		nb_table[i].result=(struct nb_result_ele *)malloc(no_tgt_value*sizeof(struct nb_result_ele));
		for(j=0;j<no_tgt_value;j++)
			nb_table[i].result[j].value=NULL;
		nb_table[i].decision=NB_UNDECIDED;
	}

	return NB_OK;

}

/* update the knowledge base */
int nb_update(int index, int tgt, void * value) {

	void *old_value;

	if((index<0) || (index >=power(2,no_attribute)))
		return NB_INDEX_ERR;

	if((tgt<0) || (tgt>=no_tgt_value))
		return NB_TARGET_ERR;

	if(nb_table==NULL)
		return NB_INIT_ERR;


	old_value=nb_table[index].result[tgt].value;

/*
	if(old_value==NULL)
		nb_table[index].result[tgt].value=value;

	else
*/
		nb_table[index].result[tgt].value=(*nb_equation)((void *)value, (void *)nb_table[index].result[tgt].value);

	free(old_value);


	return NB_OK;


}

/* ask the knowledge base for a suggestion */
int nb_suggest(int index) {

	int i;

	int suggest;

	void *preference;

        if((index<0) || (index >=power(2,no_attribute)))
                return NB_INDEX_ERR;

        if(nb_table==NULL)
                return NB_INIT_ERR;


	if(nb_table[index].decision!=NB_UNDECIDED)
		return nb_table[index].decision;

	for(i=0;i<no_tgt_value-1;i++) 
		if(nb_table[index].result[i].value==NULL)
			break;

	if(i>=0)
		return i;  /* suggest the first NULL case to explore */


	/* no NULL case */
	
	preference=nb_table[index].result[0].value;
	suggest=0;

	for(i=1;i<no_tgt_value;i++) 
		if((*nb_compare)(nb_table[index].result[i].value,preference)==1) {
			preference=nb_table[index].result[i].value;
			suggest=i;
		}

	return i; /* suggest the best case */

}

/* set the knowledge base at idx to be undecided */
int nb_undecide(int idx) {

	int i;

        if(((idx<0) || (idx >=power(2,no_attribute))) && (idx!=NB_ALL_ENTRY))
                return NB_INDEX_ERR;

        if(nb_table==NULL)
                return NB_INIT_ERR;


	if(idx==NB_ALL_ENTRY) {

		for(i=0;i<power(2,no_attribute);i++)

			nb_table[i].decision=NB_UNDECIDED;

	}
	else
		nb_table[idx].decision=NB_UNDECIDED;

	return NB_OK;

}

/* make the knowledge base at idx decided with its best performance result */
int nb_decide(int idx, int flag) {


/* idx==NB_ALL_ENTRY update all records in the table
   flag=NB_IGNORE_NULL ignore NULL cases and make decision base on others; 
	  all NULL then random
   flag=NB_SKIP_NULL skip cases with NULL value.... decide next time */

	int i,j,k;
	int uidx,lidx;
	void * preference;
	int suggest;

        if(((idx<0) || (idx >=power(2,no_attribute))) && (idx!=NB_ALL_ENTRY))
                return NB_INDEX_ERR;

        if(nb_table==NULL)
                return NB_INIT_ERR;


	lidx=0;
	uidx=power(2,no_attribute);

	if(idx!=NB_ALL_ENTRY) {
		lidx=idx;
		uidx=idx+1;
	}

	for(i=lidx;i<uidx;i++) {

		if(nb_table[i].decision != NB_UNDECIDED)
			continue;
	
		for(j=no_tgt_value-1;j>=0;j--) 
			if(nb_table[i].result[j].value==NULL)
				break;

		if((j>=0)&&(flag==NB_SKIP_NULL))   /* exist NULL and skip */
			continue;
	
		else if ((j>=0) &&(flag==NB_IGNORE_NULL)) {/* exist NULL, make decision */

			for(j=0;j<no_tgt_value;j++)
				if(nb_table[i].result[j].value!=NULL)
					break;

			if(j<no_tgt_value) {
				preference=nb_table[i].result[j].value;
				suggest=j;
			}
			else {  /* all NULL, random pick one */

				nb_table[i].decision= 1+(int) (no_tgt_value*(float)rand()/(RAND_MAX+1.0));
				continue;

			}


			for(k=j+1;k<no_tgt_value;k++)
				if((nb_table[i].result[k].value!=NULL)&&((*nb_compare)(nb_table[i].result[k].value,preference)==1)) {

					preference=nb_table[i].result[k].value;
					suggest=k;
				}

			nb_table[i].decision=suggest;


		}
		else if(j<0)   {/* no NULL, make decision */

			preference=nb_table[i].result[0].value;
			suggest=0;
			for(j=1;j<no_tgt_value;j++) {

				if((*nb_compare)(nb_table[i].result[j].value,preference)==1) {
					preference=nb_table[i].result[j].value;
					suggest=j;
				}
			}
			nb_table[i].decision=suggest;
			



		}
			
	}

	return NB_OK;

}

int nb_getindex(int no,...) {

/* no: number of bits
   LSB first */

/*TODO: lack of error checking */

	va_list ap;
	int i,d,t;

	t=0;
	va_start(ap,no);
	for(i=0;i<no;i++) {
		d=va_arg(ap,int);
		t+=d*power(2,i);
	}
	va_end(ap);

	return (t);

}

/* store the knowledge base into a file */
int nb_dumptable(char *filename,int (*dumpvoid)(FILE *,void *)) {

	FILE *out;
	int i,j;

	if((out=fopen(filename,"w"))==NULL) {
	
		return NB_FILE_ERR;
	}

	if(dumpvoid==NULL)
		return NB_FUNC_ERR;

	fprintf(out,"%d %d\n",no_attribute,no_tgt_value);
        for(i=0;i<power(2,no_attribute);i++) {
		fprintf(out,"%d ",nb_table[i].decision);
                for(j=0;j<no_tgt_value;j++) {
			if(nb_table[i].result[j].value!=NULL) {
				fprintf(out,"D");
                        	(*dumpvoid)(out, nb_table[i].result[j].value);
			}
			else
				fprintf(out,"N");
			fprintf(out," ");
		}
                fprintf(out,"\n");
        }


	fclose(out);

	return NB_OK;

}

/* restore the knowledge base from a file */
int nb_loadtable(char *filename, void *(*loadvoid)(FILE *)) {

	FILE *in;
	int i,j;
	char dt;

        if((in=fopen(filename,"r"))==NULL) {

                return NB_FILE_ERR;
        }
	if(loadvoid==NULL)
		return NB_FUNC_ERR;

        fscanf(in,"%d %d\n",&no_attribute,&no_tgt_value);
        for(i=0;i<power(2,no_attribute);i++) {
                fscanf(in,"%d ",&(nb_table[i].decision));
                for(j=0;j<no_tgt_value;j++) {
			fscanf(in,"%c",&dt);
			if(dt=='D') 
                        	nb_table[i].result[j].value=(*loadvoid)(in);
			else if(dt=='N')
				nb_table[i].result[j].value=NULL;
                        fscanf(in," ");
                }
                fscanf(in,"\n");
        }


        fclose(in);

        return NB_OK;


	


}

/* get the result value at index from knowledge base */
void * nb_getvalue(int index, int tgt) {

        if((index<0) || (index >=power(2,no_attribute)))
                return NULL;

        if((tgt<0) || (tgt>=no_tgt_value))
                return NULL;

        if(nb_table==NULL)
                return NULL;


	return nb_table[index].result[tgt].value;



}

/* set the knowledge base at index with value */
int nb_setdecision(int index, int value) {

        if((index<0) || (index >=power(2,no_attribute)))
                return NB_INDEX_ERR;

        if((value<0) || (value>no_attribute))
                return NB_TARGET_ERR;

        if(nb_table==NULL)
                return NB_INIT_ERR;


	nb_table[index].decision=value;

	return NB_OK;

}


/* print the knowledge base table */
int print_nbtable(char *metric_type) {

	int i,j;

	for(i=0;i<power(2,no_attribute);i++) {
		printf("[%d] ",i);
		for(j=0;j<no_tgt_value;j++)
			if(nb_table[i].result[j].value!=NULL) {
				if(strcmp(metric_type,"float")==0)
				printf("%f ",*(float *)nb_table[i].result[j].value);
				else	
				printf("%d ",*(int *)nb_table[i].result[j].value);
			}
			else
				printf("N ");
		if(nb_table[i].decision==NB_UNDECIDED)
			printf("-> undecided\n");
		else
			printf("-> %d\n",nb_table[i].decision);
	}
	return NB_OK;


}

#ifdef NB_DEBUG
			
		

int dvoid(FILE *f, void * data) {

	fprintf(f,"%d",*(int *)data);
	return NB_OK;

}

void *lvoid(FILE *f) {

	int *i;

	i=(int *)malloc(sizeof(int));
	fscanf(f,"%d",i);

	return i;

}

int intcomp(void *i, void *j) {

	if(*(int *)i>*(int *)j)
		return 1;
	else
		return 0;

}

int * intequa(int *new, int *old) {

	float alpha=0.5;

	if(old!=NULL)
		*new=(int)((1.0-alpha)*((float)(*new))+alpha*((float)(*old)));

	return new;
}


	


int main(void ) {

	int r1=3;
	int r2=5;
	int r3=4;
	int r4=8;

	printf("[[%d]]\n",nb_init(5,3,intcomp,(void *)intequa));
	nb_update(nb_getindex(5,1,1,0,1,0),0,&r1);
	nb_update(nb_getindex(5,1,1,0,1,0),1,&r2);
	nb_update(nb_getindex(5,1,1,0,1,0),2,&r3);
	
	nb_update(nb_getindex(5,1,1,0,1,0),2,&r4);

	print_nbtable("int");
	printf("here\n");
	printf("suggestion(%d)=%d\n",nb_getindex(5,1,1,0,1,0),nb_suggest(nb_getindex(5,1,1,0,1,0)));

	nb_decide(nb_getindex(5,0,1,0,1,0),NB_IGNORE_NULL);

	nb_decide(nb_getindex(5,0,1,1,1,0),NB_IGNORE_NULL);

	printf("suggestion(%d)=%d\n",nb_getindex(5,0,1,1,1,0),nb_suggest(nb_getindex(5,0,1,1,1,0)));

	nb_decide(NB_ALL_ENTRY,NB_SKIP_NULL);

	nb_dumptable("test.out",dvoid);

	nb_loadtable("test2.out",lvoid);

	nb_decide(28,NB_IGNORE_NULL);


	print_nbtable("int");

	

	return 0;

}

#endif
