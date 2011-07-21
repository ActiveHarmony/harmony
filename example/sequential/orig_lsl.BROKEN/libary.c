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

/* $Revision: 1.14 $ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#include "errorcode.h"
#include "libll.h"
#include "libary.h"
#ifdef LINUX
#include <hrtime.h>
#endif


#define max(a,b) ((a)>(b)?(a):(b))
#define DEFAULT_TABLE_NO 20

struct ary_table_meta *all_tables;

int *max_tables;
static int tid;

static float insert_delete_time, search_time;

void print_table(void *data);

void m_time(float *result) {

/*
        static struct timeval t1,t2;
	struct timezone tz;

        if(result==NULL)
                gettimeofday(&t1,&tz);

        else {
                gettimeofday(&t2,&tz);
                *result=(float)(t2.tv_sec-t1.tv_sec)+((float)(t2.tv_usec-t1.tv_usec))/1000000;
	}
*/

#if defined(LINUX)

        static int first=1;
        static hrtime_t c1,c2;
        static struct hrtime_struct *hr;
        int error;

        if(first==1) {

                first=0;

                error = hrtime_init();
                if (error < 0)
                {
                        printf("Error: %s\n", strerror(error));
                        exit(1);
                }

                error = get_hrtime_struct(0, &hr);
                if (error < 0)
                {
                        printf("Error: %s\n", strerror(error));
                        exit(1);
                }
        }

        if(result==NULL)
                get_hrvtime(hr, &c1);

        else {
                get_hrvtime(hr, &c2);
                *result=(float)(c2-c1)/1000000000.0;
        }

#else


	static hrtime_t t1,t2;
        if(result==NULL) {
                t1=gethrvtime();
	//	printf("[[%lld]]\n",t1);
	}

        else {
                t2=gethrvtime();
		*result=(float)((t2-t1)/1000000000.0);

	//	printf("<<%lld %lld %lld>>\n",t2,t1,t2-t1);
	}

#endif

        return;
}



void ary_init(struct ary_table_meta **data) {

	int i;

	tid=0;

	all_tables=(struct ary_table_meta *)malloc(DEFAULT_TABLE_NO*sizeof(struct ary_table_meta));

	all_tables[0].notables=DEFAULT_TABLE_NO;
	max_tables=&(all_tables[0].notables);
	for(i=0;i<*max_tables;i++) {
		all_tables[i].id=-1;
		all_tables[i].max_y=0;
		all_tables[i].table=NULL;
	}

	if(data!=NULL)
		*data=all_tables;


}

int ary_newtable() {

	int i;
	struct ary_table_meta *new_tables;

	tid++;

	for(i=0;i<*max_tables;i++) 
		if(all_tables[i].id == -1) 
			break;

	if(i<*max_tables) { // there is still empty space

		all_tables[i].id=tid;
		//all_tables[i].table=NULL;

	}
	else  { /* re-allocate all_tables */

		//(*max_tables)+=1;
		new_tables=(struct ary_table_meta *)malloc((*max_tables+1)*sizeof(struct ary_table_meta));
		memcpy(new_tables,all_tables,(*max_tables)*sizeof(struct ary_table_meta));
		free(all_tables);
		all_tables=new_tables;
		(*max_tables)++;
		//if(new_tables!=all_tables) {
		//	if(all_tables!=NULL) free(all_tables);
		//	all_tables=new_tables;
		//}
		all_tables[*max_tables-1].id=tid;
		all_tables[*max_tables-1].max_y=0;
		all_tables[*max_tables-1].table=NULL;

	}
	return tid;

}

int ary_insert(int tid, int x, int y, void *e){

	int i,j,k,nszy,nszx;
	struct ary_table_ele *t,*tbl,*tc;

	/* get performance measurement */
	//printf("ary_insert is called\n");

	m_time(NULL);

	for(i=0;i<*max_tables;i++)
		if(all_tables[i].id==tid)
			break;
	if(i==*max_tables) 
		return NO_SUCH_TABLE;

	t=all_tables[i].table;

	nszy=max(all_tables[i].max_y,y+1);
	if(y>=all_tables[i].max_y) {
		tbl=(struct ary_table_ele *)malloc((y+1)*sizeof(struct ary_table_ele));
		memcpy(tbl,t,all_tables[i].max_y*sizeof(struct ary_table_ele));
		all_tables[i].table=tbl;
		free(t);
		t=tbl;
	        for(j=all_tables[i].max_y;j<nszy;j++) {
        	        t[j].ele=NULL;
        	        t[j].max_x=0;
        	}
		all_tables[i].max_y=y+1;
		//nszy=y+1;
	}

/*	nszy=max(all_tables[i].max_y,y+1);
	tbl=(struct ary_table_ele *)realloc(t,nszy*sizeof(struct ary_table_ele));
	if(tbl!=t) {
		if(t!=NULL)free(t);
		t=tbl;
	}
	for(j=all_tables[i].max_y;j<nszy;j++) {
		t[j].ele=NULL;
		t[j].max_x=0;
	}
	all_tables[i].table=t;
	all_tables[i].max_y=nszy;
*/
	
	tc=t[y].ele;
	nszx=max(t[y].max_x,x+1);
	if(x>=t[y].max_x) {
                tbl=(struct ary_table_ele *)malloc((x+1)*sizeof(struct ary_table_ele));
                memcpy(tbl,tc,t[y].max_x*sizeof(struct ary_table_ele));
                t[y].ele=tbl;
		free(tc);
                tc=tbl;
	        for(k=t[y].max_x;k<nszx;k++) {
                	tc[k].ele=NULL;
                	tc[k].max_x=0;
        	}

                t[y].max_x=x+1;
                //nszx=x+1;
        }


	
/*	
	nszx=max(t[j].max_x,x+1);
	tbl=(struct ary_table_ele *)realloc(tc,nszx*sizeof(struct ary_table_ele));
	if(tbl!=tc) {
		if(tc!=NULL)free(tc);
		tc=tbl;
	}
	for(k=t[j].max_x;k<nszx;k++) {
		tc[k].ele=NULL;
		tc[k].max_x=0;
	}
	t[y].max_x=nszx;
	t[y].ele=tc;
*/

	tc[x].ele=e;

	m_time(&insert_delete_time);

	
	return OK;
	


}

int ary_delete(int tid, int x, int y) {

	int i,j,k;
	struct ary_table_ele *t,*tc,*tbl;

	m_time(NULL);


        for(i=0;i<*max_tables;i++)
                if(all_tables[i].id==tid)
                        break;
        if(i==*max_tables)
                return NO_SUCH_TABLE;

	t=all_tables[i].table;
	if(t==NULL)
		return NO_SUCH_ELEMENT;
	if(y>=all_tables[i].max_y)
		return NO_SUCH_ELEMENT;
	tc=(struct ary_table_ele *)t[y].ele;
	if(tc==NULL)
		return NO_SUCH_ELEMENT;
	if(x>=t[y].max_x)
		return NO_SUCH_ELEMENT;

	tc[x].ele=NULL;  // remove the item


	if(x==t[y].max_x-1) { // last one in this column
		for(j=x;j>=0;j--)
			if(tc[j].ele !=NULL)
				break;

		t[y].max_x=j+1;
                tbl=(struct ary_table_ele *)malloc(t[y].max_x*sizeof(struct ary_table_ele));
		memcpy(tbl,t[y].ele,t[y].max_x*sizeof(struct ary_table_ele));
		free(t[y].ele);
		t[y].ele=tbl;
		//if(tbl!=t[y].ele) {
			//if(t[y].ele!=NULL) free(t[y].ele);	
		//	t[y].ele=tbl;
		//}


		if(y==all_tables[i].max_y-1) { 

		//printf ("%d %d\n",y,all_tables[i].max_y-1);
		/* the rightmost column is now empty */
			for(k=y-1;k>=0;k--) {
				if(t[k].ele != NULL)
					break;
			}
			all_tables[i].max_y=k+1;
			tbl=(struct ary_table_ele *)malloc(all_tables[i].max_y*sizeof(struct ary_table_ele));
			memcpy(tbl,all_tables[i].table,all_tables[i].max_y*sizeof(struct ary_table_ele));
			free(all_tables[i].table);
			all_tables[i].table=tbl;

			//if(tbl!=all_tables[i].table) {
				//if(all_tables[i].table!=NULL)free(all_tables[i].table);
			//	all_tables[i].table=tbl;
			//}

		}

	}


	m_time(&insert_delete_time);

		
	return OK;
}

int ary_search(int tid, void *e, int size, int *x, int *y) {


	int j,k;
	struct ary_table_ele *tc,*te;
	static int i,lastx,lasty;
	static void *laste;

	m_time(NULL);

if(e!=NULL) {	
        for(i=0;i<*max_tables;i++)
                if(all_tables[i].id==tid)
                        break;
        if(i==*max_tables) {
		*x=*y=-1;
                return NO_SUCH_TABLE;
	}

	if(all_tables[i].table!=NULL) {
		te=all_tables[i].table;
		for(j=0;j<all_tables[i].max_y;j++) {
			tc=te[j].ele;
			for(k=0;k<te[j].max_x;k++){
				if((tc[k].ele!=NULL)&&(memcmp(tc[k].ele,e,size)==0)) {
					*x=k;
					*y=j;					
					lastx=k;
					lasty=j;
					laste=e;

					m_time(&search_time);

					return OK;
				}
			}
		}
	}
	*x=*y=-1;
	return NO_SUCH_ELEMENT;
}
else {
	j=lasty;
	te=all_tables[i].table;
	tc=te[j].ele;
	for(k=lastx+1;k<te[j].max_x;k++) {
                if((tc[k].ele!=NULL)&&(memcmp(tc[k].ele,laste,size)==0)) {
                                        *x=k;
                                        *y=j;
                                        lastx=k;
                                        lasty=j;

					m_time(&search_time);


                                        return OK;
                }
	}
	
	for(j=lasty+1;j<all_tables[i].max_y;j++) {
		tc=te[j].ele;
		for(k=0;k<te[j].max_x;k++) {
			if((tc[k].ele!=NULL)&&(memcmp(tc[k].ele,laste,size)==0)) {
                                        *x=k;
                                        *y=j;
                                        lastx=k;
                                        lasty=j;

					m_time(&search_time);

                                        return OK;
                         }
		}
	}

	*x=*y=-1;
	return NO_SUCH_ELEMENT;
}

}

void * ary_elementAt(int tid, int x, int y) {

	int i;
	struct ary_table_ele *t,*tc;

        for(i=0;i<*max_tables;i++) {
                if(all_tables[i].id==tid)
                        break;
	}

        if(i==*max_tables) 
                return NULL;

	t=all_tables[i].table;
	if(t==NULL)
		return NULL;

	if(y>=all_tables[i].max_y)
		return NULL;
	tc=t[y].ele;

	if(tc==NULL)
		return NULL;
	if(x>=t[y].max_x)
		return NULL;
	return tc[x].ele;

}

int ary_freetable(int tid) {

	int i,j;
	struct ary_table_ele *t,*tc;

        for(i=0;i<*max_tables;i++)
                if(all_tables[i].id==tid)
                        break;
        if(i==*max_tables)
                return NO_SUCH_TABLE;

	t=all_tables[i].table;
	for(j=0;j<all_tables[i].max_y;j++) {
		tc=t[j].ele;
		free(tc);
	}
	free(t);
	all_tables[i].table=NULL;
	all_tables[i].id=-1;

	return OK;

}

/* TODO : under construction */

/*
void ary_from_fary(void **data) {

	extern void* f_ary_elementat_(int*,int*,int*);
	extern int f_ary_maxindex_(int *,int *, int*);
	extern int f_ary_iterate_tid_(int*);

	int flag=0;
	int r_id,l_id;
	int i,j,x,y;
	void *p;

	if((r_id=f_ary_iterate_tid_(&flag))==-1)
		return ;

	ary_init((struct ary_table_meta**)data);

	flag=1;

	do {
		l_id=ary_newtable();
		f_ary_maxindex_(&r_id,&x,&y);
		for(i=1;i<=x;i++)
			for(j=1;j<=y;j++) {
				p=f_ary_elementat_(&r_id,&i,&j);
				if(p!=NULL)
					ary_insert(l_id,i,j,p);
			}
	}
		
	while((r_id=f_ary_iterate_tid_(&flag))==-1);

	return ;

}
*/

void ary_fromll(void **data) {

        struct ary_table_meta *ary_tab;
        struct ll_table_meta *ll_tab,*nt,*ns;

	struct ary_table_ele *s,*sc;
	struct ll_table_ele *t,*tc,*tr,*tx,*ty;

	int notables,max_y,max_x;

	int max_tid=0;

	int i,j,k;

	ll_tab=(struct ll_table_meta *)(*data);

	notables=0;
	for(nt=ll_tab->next;nt!=ll_tab;nt=nt->next)
		notables++;
	

	if(notables <20)
		notables=20;

	ary_tab=(struct ary_table_meta *)malloc(notables*sizeof(struct ary_table_meta));

	ary_tab[0].notables=notables;
	max_tables=&(ary_tab[0].notables);

	i=0;
	for(nt=ll_tab->next;nt!=ll_tab;nt=nt->next) {

		ary_tab[i].id=nt->id;
		ary_tab[i].max_y=0;
		ary_tab[i].table=NULL;

		if(nt->id > max_tid)
			max_tid=nt->id;

		t=nt->table;
		if(t != NULL) {

			max_y=-1;
			for(tc=t->rnext;tc!=t;tc=tc->rnext) 
				max_y=max(max_y,tc->y);

			ary_tab[i].max_y=max_y+1;

			s=(struct ary_table_ele *)malloc((max_y+1)*sizeof(struct ary_table_ele ));
			for(j=0;j<(max_y+1);j++) {
				s[j].ele=NULL;
				s[j].max_x=0;
			}

			ary_tab[i].table=s;

			for(tc=t->rnext;tc!=t;tc=tc->rnext) {

				max_x=-1;
				for(tr=tc->lnext;tr!=tc;tr=tr->lnext)
					max_x=max(max_x,tr->x);

				s[tc->y].max_x=max_x+1;

				sc=(struct ary_table_ele *)malloc((max_x+1)*sizeof(struct ary_table_ele));

				for(k=0;k<(max_x+1);k++) {
					sc[k].ele=NULL;
					sc[k].max_x=0;
				}

				s[tc->y].ele=sc;

				for(tr=tc->lnext;tr!=tc;tr=tr->lnext) 
					sc[tr->x].ele=tr->ele;

				tr=tc->lnext;
				while(tr!=tc) {
					tx=tr->lnext;
					free(tr);
					tr=tx;
				}
				
			}

			tc=t->rnext;
			while(tc!=t) {
				ty=tc->rnext;
				free(tc);
				tc=ty;
			}
			free(t);
		}
		


		i++;

	}

	tid=max_tid;

	nt=ll_tab->next;
	while(nt!=ll_tab) {
		ns=nt->next;
		free(nt);
		nt=ns;
	}
	free(nt);

	*data=ary_tab;
	all_tables=ary_tab;

	return;

}

int ary_tablesize(int tid, void *data) {

        int i,j;
        struct ary_table_ele *tc;

        struct ary_table_meta *datatable;
        int *max_tables;

	int size=0;

        if(data!=NULL)
                datatable=(struct ary_table_meta *)data;
        else
                datatable=all_tables;

	max_tables=&(datatable[0].notables);

        for(i=0;i<*max_tables;i++)
                if(all_tables[i].id==tid)
                        break;
        if(i==*max_tables)
                return NO_SUCH_TABLE;

	tc=all_tables[i].table;

	if(tc==NULL)
		return size;

	size=datatable[i].max_y*sizeof(struct ary_table_ele);

	//printf("max_y=%d\n",datatable[i].max_y);

        for(j=0;j<datatable[i].max_y;j++) {

		//printf("\t%dmax_x=%dx%d\n",j,tc[j].max_x,sizeof(struct ary_table_ele));
		size+=tc[j].max_x*sizeof(struct ary_table_ele);

	}

	return size;

}

int ary_memUsed(void *data) {

	int i;

        struct ary_table_meta *datatable;
        int *max_tables;

	int size=0;

        if(data!=NULL)
                datatable=(struct ary_table_meta *)data;
        else
                datatable=all_tables;

        max_tables=&(datatable[0].notables);

        size=(*max_tables)*sizeof(struct ary_table_meta);

	//printf("%d,%d,%d\n",(*max_tables),sizeof(struct ary_table_meta),size);

        for(i=0;i<*max_tables;i++) 
		size+=ary_tablesize(datatable[i].id,datatable);

	return size;
}

void print_table(void *data) {

	int i,j,k;
	struct ary_table_ele *tc,*tr;

	struct ary_table_meta *datatable;
	int *max_tables;

	if(data!=NULL)
		datatable=(struct ary_table_meta *)data;
	else
		datatable=all_tables;

	max_tables=&(datatable[0].notables);

	printf("begin\n");
	for(i=0;i<*max_tables;i++) {

		if(datatable[i].table!=NULL) {
	
			tc=datatable[i].table;	
			for(j=0;j<datatable[i].max_y;j++) {
				tr=(struct ary_table_ele *)tc[j].ele;
				for(k=0;k<tc[j].max_x;k++) {
					if(tr[k].ele !=NULL)
						printf("[%d,%d]:%d\n",k,j,*(int *)tr[k].ele);
				}
			}
		}
	}
	printf("end\n");
}

float ary_sTime() {

	return (float)search_time;
}

float ary_idTime() {

	//printf("<<%f>>\n",insert_delete_time);

	return (float)insert_delete_time;
}

int ary_EstimateMem(int datasize) {

	int x=(int)sqrt((double)datasize);

	x=x*sizeof(struct ary_table_meta);

	return (x+datasize*sizeof(struct ary_table_ele));
}


float ary_EstimateID(int datasize) {

	int tid;
	float t;
	int x=(int)sqrt((double)datasize),e=5;

	float et;	


	tid=ary_newtable();

        m_time(NULL);

	ary_insert(tid,x,x,&e);

	m_time(&et);

	t=(float)et;

	m_time(NULL);

	ary_delete(tid,x,x);

	m_time(&et);

	t=(t+(float)et)/2;

	ary_freetable(tid);

	return t;

}


float ary_EstimateS(int datasize) {
        int tid;
        float t;
        int x=(int)sqrt((double)datasize),e=5;

	float et;


        tid=ary_newtable();

        ary_insert(tid,x,x,&e);        

	m_time(NULL);

	ary_search(tid,&e,sizeof(int),&x,&x);

	m_time(&et);

	t=(float)et;

	ary_delete(tid,x,x);

	ary_freetable(tid);

	return t;


}

void *ary_fromll_time(void *data) {

	float *t=malloc(sizeof(float));
	int max_tables;

	struct ary_table_meta *ary_tab;

        ary_tab=(struct ary_table_meta *)(data);

        max_tables=ary_tab[0].notables;

	/* TODO : how to estiamte? */

	*t=(float)max_tables;

	return t;

}

int ary_maxindex(int tid, int *max_x, int *max_y) {

	int i,j;

        struct ary_table_ele *t;

        /* get performance measurement */

	*max_x=*max_y=-1;

        for(i=0;i<*max_tables;i++)
                if(all_tables[i].id==tid)
                        break;
        if(i==*max_tables)
                return NO_SUCH_TABLE;

	*max_y=all_tables[i].max_y-1;

        t=all_tables[i].table;
	for(j=0;j<all_tables[i].max_y;j++) 
		*max_x=max(*max_x,(t[j].max_x-1));

	
	return OK;

}

int ary_iterate_tid(int flag) {

	static int i;
	int j;

	if(flag==0)
		i=0;

        for(j=i;j<*max_tables;j++)
                if(all_tables[j].id!=-1)
                        break;

	if(j==*max_tables) {
		i=0;
		return -1;
	}
	else {
		//if(j==*max_tables-1)
		//	i=0;
		//else
			i=j+1;

		return all_tables[j].id;

	}
}

#ifdef TEST

int main(void) {

	int t;

	int e=5;
	int p=7;
	int x,y;
	int i,j;
	int *k;

	ary_init(NULL);

	printf("ary_EstimateMem(100)=%d\n",ary_EstimateMem(100));

	t=ary_newtable();

	i=ary_iterate_tid(0);
	j=ary_iterate_tid(1);

	printf("[[%d,%d]]\n",i,j);
	printf("total size %d=%d\n",t,ary_memUsed(NULL));
	ary_insert(t,3,200,&e);

	printf("total size %d=%d\n",t,ary_memUsed(NULL));
	ary_insert(t,6,7,&e);
	ary_insert(t,3,40,&e);

	ary_maxindex(t,&x,&y);
	printf("max index = (%d,%d)\n",x,y);

	printf("begin\n");
	for(i=0;i<=x;i++)
		for(j=0;j<=y;j++)
			if ((k=ary_elementAt(t,i,j))!=NULL)
				printf("(%d,%d)=%d\n",i,j,*k);


	printf("end\n");


	printf("idtime=%f\n",ary_idTime());
	print_table(NULL);
	ary_delete(t,3,200);
	printf("total size %d=%d\n",t,ary_memUsed(NULL));
	print_table(NULL);
	ary_insert(t,6,5,&p);
	print_table(NULL);

	ary_search(t,&e,sizeof(int),&x,&y);
	printf("x=%d,y=%d\n",x,y);
        ary_search(t,NULL,sizeof(int),&x,&y);
        printf("x=%d,y=%d\n",x,y);

	ary_insert(t,60,750,&e);
        print_table(NULL);
        ary_search(t,NULL,sizeof(int),&x,&y);
        printf("x=%d,y=%d\n",x,y);

	printf("(%d,%d)=%d\n",x,y,*(int *)ary_elementAt(t,x,y));

	//ary_freetable(t);
	print_table(NULL);

	return 0;
}


#endif
