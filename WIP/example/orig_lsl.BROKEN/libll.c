/*
 * Copyright 2003-2015 Jeffrey K. Hollingsworth
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

/* $Revision: 1.13 $ 
   $Date: 2002/02/20 19:11:01 $          */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "queue2.h"
#include "errorcode.h"
#include "libll.h"
#include "libary.h"  /* used for conversion */
#ifdef LINUX
//#include <hrtime.h>
#endif

#define max(a,b) ((a)>(b)?(a):(b))


struct ll_table_meta *all_tables;

static int tid;

void print_table(void *datatable);

static float insert_delete_time,search_time;

void m_time(float *result) {

/*
 	static struct timeval t1,t2;
   struct timezone tz;

	if(result==NULL) 
		gettimeofday(&t1,&tz);

	else {
		gettimeofday(&t2,&tz);
		*result=(float)(t2.tv_sec-t1.tv_sec)+((float)(t2.tv_usec-t1.tv_usec))/1000000;

	//printf("*result=%f\n",*result);

	}



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
        if(result==NULL)
                t1=gethrvtime();

        else {
                t2=gethrvtime();
                *result=(float)((t2-t1)/1000000000.0);

	//	printf("<<%lld %lld %lld>>\n",t2,t1,t2-t1);
        }

#endif /* LINUX */

	return;
}




void ll_init(struct ll_table_meta **data) { 


	tid=0;


	InitDQ(all_tables,struct ll_table_meta);
	all_tables->id=-1;
	all_tables->table=NULL;

	if(data!=NULL)
		*data=all_tables;



}

/*
void destructor(void) {

	free(all_tables);
	all_tables=NULL;	

}
*/

int ll_newtable(/*struct ll_table_meta **data */) {

	struct ll_table_meta * nt;

	//if(data!=NULL)
	//	*data=all_tables;

	nt=(struct ll_table_meta *)malloc(sizeof(struct ll_table_meta));

	tid++;
	nt->id=tid;
	nt->table=NULL;

	InsertDQ(all_tables,nt);

	return tid;

}

struct ll_table_meta * find_ll_table_meta(int id) {
	struct ll_table_meta *i;

	for (i=all_tables->next;i!=all_tables;i=i->next)
		if(i->id == id)
			break;
	if(i->id==id)
		return i;
	else
		return NULL;
}

struct ll_table_ele *locate_ele_y(struct ll_table_ele *r,int y) {

	struct ll_table_ele *i;

	for(i=r->rnext;i!=r;i=i->rnext)
		if(i->y==y) 
			break;
	if(i->y==y)
		return i;
	else
		return NULL;
}

struct ll_table_ele *locate_ele_x(struct ll_table_ele *r, int x) {

	struct ll_table_ele *i;

        for(i=r->lnext;i!=r;i=i->lnext)
                if(i->x == x)
                        break;
        if(i->x==x)
                return i;
        else 
                return NULL;
}

	

int ll_insert(int tid, int x, int y, void *e){
	
	struct ll_table_meta *tm;
	struct ll_table_ele *t,*tc,*tr;


	m_time(NULL);

	tm=find_ll_table_meta(tid);
	if(tm==NULL)
		return NO_SUCH_TABLE;

	t=tm->table;
	if(t==NULL) {
		//InitDQ(t,struct ll_table_ele);
		t=(struct ll_table_ele *)malloc(sizeof(struct ll_table_ele));
		t->lnext=t->rnext=t->rprev=t->lprev=t;
		t->x=-1;
		t->y=-1;
		t->ele=NULL;
		tm->table=t;
	}
	tc=locate_ele_y(t,y);
	if(tc==NULL) {
		tr=(struct ll_table_ele *)malloc(sizeof(struct ll_table_ele));
		tr->x=-1;
		tr->y=y;
		tr->ele=NULL;
		tr->rprev=tr->rnext=NULL;
		tr->lnext=tr->lprev=tr;
		InsertDQ_r(t,tr);
		tc=tr;
	}
	t=tc;
	tc=locate_ele_x(t,x);
	if(tc==NULL) {
		tr=(struct ll_table_ele *)malloc(sizeof(struct ll_table_ele));
                tr->x=x;
                tr->y=y;
                tr->ele=NULL;
		tr->rprev=tr->rnext=tr;
		tr->lnext=tr->lprev=NULL;
                InsertDQ_l(t,tr);
		tc=tr;
        }
	tc->ele=e;





	m_time(&insert_delete_time);


	return OK;
}

int ll_delete(int tid, int x, int y) {


    struct ll_table_meta *tm;
    struct ll_table_ele *t,*tc,*tr;
    
    m_time(NULL);
    
    tm=find_ll_table_meta(tid);
    if(tm==NULL)
        return NO_SUCH_TABLE;
    
    t=tm->table;
    if(t==NULL) 
        return NO_SUCH_ELEMENT; 
    tc=locate_ele_y(t,y);
    if(tc==NULL) 
        return NO_SUCH_ELEMENT;
    tr=locate_ele_x(tc,x);
    if(tr==NULL) 
        return NO_SUCH_ELEMENT;
    
    DelDQ_l(tr);
    free(tr);
    if(EmptyDQ_l(tc)) {
        DelDQ_r(tc);
        free(tc);
        if(EmptyDQ_r(t)) {
            free(t);
            tm->table=NULL;
        }
    }
    
    m_time(&insert_delete_time);
    
    
    return OK;
    
    
}


int ll_search(int tid, void *e, int size, int *x, int *y) {
    
    
    struct ll_table_meta *tm;
    struct ll_table_ele *tx,*ty;
    
    static struct ll_table_ele *te,*lastx,*lasty;
    static void *laste;
    
    m_time(NULL);
    
    printf("ll_search is called\n");
    
    if(e!=NULL) {
        
        tm=find_ll_table_meta(tid);
        if(tm==NULL) {
            *x=*y=-1;
            return NO_SUCH_TABLE;
        }
        
        if(tm->table!=NULL) {
            te=tm->table;
            for(ty=te->rnext;ty!=te;ty=ty->rnext) {
                for(tx=ty->lnext;tx!=ty;tx=tx->lnext) {
                    
                    if(memcmp(tx->ele,e,size)==0) {
                        *x=tx->x;
                        *y=ty->y;
                        lastx=tx;
                        lasty=ty;
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
        
        ty=lasty;
        for(tx=lastx->lnext;tx!=ty;tx=tx->lnext) {
            if(memcmp(tx->ele,laste,size)==0) {
                *x=tx->x;
                *y=ty->y;
                lastx=tx;
                lasty=ty;
                
                m_time(&search_time);
                
                return OK;
            }
        }
        for(ty=lasty->rnext;ty!=te;ty=ty->rnext) {
            for(tx=ty->lnext;tx!=ty;tx=tx->lnext) {
                if(memcmp(tx->ele,laste,size)==0) {
                    *x=tx->x;
                    *y=ty->y;
                    lastx=tx;
                    lasty=ty;
                    
                    m_time(&search_time);
                    
                    
                    return OK;
                }
            }
        }
        *x=*y=-1;
        return NO_SUCH_ELEMENT;
    }    
}


void * ll_elementAt(int tid, int x, int y) {

        struct ll_table_meta *tm;
        struct ll_table_ele *t,*tc,*tr;

        tm=find_ll_table_meta(tid);
        if(tm==NULL)
                return NULL;

        t=tm->table;
        if(t==NULL)
                return NULL;
        tc=locate_ele_y(t,y);
        if(tc==NULL)
                return NULL;
        tr=locate_ele_x(tc,x);
        if(tr==NULL)
                return NULL;

	return tr->ele;

}



int ll_freetable(int tid) {


        struct ll_table_meta *tm;
        struct ll_table_ele *te,*tx,*ty,*tp,*tq;;

        tm=find_ll_table_meta(tid);
        if(tm==NULL)
                return NO_SUCH_TABLE;

        te=tm->table;
	ty=te->rnext;
	while(ty!=te) {
		tp=ty->rnext;

		tx=ty->lnext;
		while(tx!=ty) {
			tq=tx->lnext;
			free(tx);
			tx=tq;
		}

		free(ty);
		ty=tp;
	}
	free(te);

	tm->table=NULL;
	DelDQ(tm);
	free(tm);


	return OK;


}

void ll_fromary(void **data) {

	struct ary_table_meta *ary_tab;
	struct ll_table_meta *ll_tab,*nt;

	struct ary_table_ele *s,*sc;
	struct ll_table_ele *t,*tc,*tr;

	int i,j,k;
	int max_tables,max_tid=0;


	ary_tab=(struct ary_table_meta *)(*data);	

	max_tables=ary_tab[0].notables;


        InitDQ(ll_tab,struct ll_table_meta);
        ll_tab->id=-1;
        ll_tab->table=NULL;
	for(i=0;i<max_tables;i++) {
		if(ary_tab[i].table!=NULL) {

        		nt=(struct ll_table_meta *)malloc(sizeof(struct ll_table_meta));

			/* handle the meta data for this table */
        		nt->id=ary_tab[i].id;

			if(nt->id > max_tid)
				max_tid=nt->id;
			nt->table=NULL;

			/* convert the table contents */
			if(ary_tab[i].table != NULL) {
		
				/* initial the first one of column */
				t=(struct ll_table_ele *)malloc(sizeof(struct ll_table_ele));  
                                t->lnext=t->rnext=t->rprev=t->lprev=t; 
                                t->x=t->y=-1;  
                                t->ele=NULL;   
                                nt->table=t;    

				s=ary_tab[i].table;

				for(j=0;j<ary_tab[i].max_y;j++) {


				if (s[j].max_x !=0 ) {


					tc=(struct ll_table_ele *)malloc(sizeof(struct ll_table_ele));
					tc->x=-1;
                			tc->y=j;
                			tc->ele=NULL;
                			tc->rprev=tc->rnext=NULL;
                			tc->lnext=tc->lprev=tc;
                			InsertDQ_r(t,tc);

					sc=s[j].ele;
	
					for(k=0;k<s[j].max_x;k++) {

					if(sc[k].ele != NULL) {

						tr=(struct ll_table_ele *)malloc(sizeof(struct ll_table_ele)); 
						tr->x=k;
						tr->y=j;
						tr->ele=sc[k].ele;
						tr->rprev=tr->rnext=tr;
						tr->lprev=tr->lprev=NULL;
						InsertDQ_l(tc,tr);

					}
					}
					free(sc);


				}
				}

				free(s);
				ary_tab[i].table=NULL;
				ary_tab[i].id=-1;
			
			}

        		InsertDQ(ll_tab,nt);

		}
		
	}

	tid=max_tid;


	free(ary_tab);


	*data=ll_tab;
	all_tables=ll_tab;

	return;

}



void print_table(void *datatable) {

struct ll_table_meta *tm, *rt;
struct ll_table_ele *te,*ty,*tx;

if(datatable==NULL)
	rt=all_tables;
else
	rt=datatable;

printf("begin\n");
for (tm=rt->next;tm!=rt;tm=tm->next) {

	if(tm->table!=NULL) {
		te=tm->table;
		for(ty=te->rnext;ty!=te;ty=ty->rnext) {
			for(tx=ty->lnext;tx!=ty;tx=tx->lnext) {
				printf("[%d,%d]:%d\n",tx->x,tx->y,*(int *)tx->ele);
			}
		}
	}
}
printf("end\n");
}

int ll_tablesize(int tid ) {

struct ll_table_meta *rt;
struct ll_table_ele *te,*ty,*tx;

int y,x;
int size=0;

rt=find_ll_table_meta(tid);
if(rt==NULL)
	return size;

te=rt->table;
if(te==NULL)
	return size;

y=0;
for (ty=te->rnext;ty!=te;ty=ty->rnext) {
	y++;
	tx=ty->ele;
	x=0;
	if(ty->ele != NULL) {
		for(tx=ty->lnext;tx!=ty;tx=tx->lnext)
			x++;
	}
	size+=x*sizeof(struct ll_table_ele);
}
size+=y*sizeof(struct ll_table_ele);

return size;
}

int ll_EstimateMem(int datasize) {

	return (datasize*sizeof(struct ll_table_ele));

}

float ll_EstimateID(int datasize) {

        int tid;
        float t;
        int x=(int)sqrt((double)datasize),e=5;

	float et;

        //struct timeval t1,t2;
        //struct timezone tz;

        tid=ll_newtable();

        //gettimeofday(&t1,&tz);
	m_time(NULL);

        ll_insert(tid,x,x,&e);

        //gettimeofday(&t2,&tz);
	m_time(&et);

        t=(float)et;

        //gettimeofday(&t1,&tz);
	m_time(NULL);

        ll_delete(tid,x,x);

        //gettimeofday(&t2,&tz);
	m_time(&et);

        t=(t+(float)et)/2;

        ll_freetable(tid);

        return t;

}

float ll_EstimateS(int datasize) {

    int tid;
    float t;
    int x=(int)sqrt((double)datasize),e=5;
    float et;
    
    
    tid=ll_newtable();
    
    ll_insert(tid,x,x,&e);
    
    m_time(NULL);
    
    ll_search(tid,&e,sizeof(int),&x,&x);
    
    m_time(&et);
    
    t=(float)et;
    
    ll_delete(tid,x,x);
    
    ll_freetable(tid);
    
    return t;
    
    
}

int ll_memUsed(void *datatable) {

    int size=0;
    
    struct ll_table_meta *tm, *rt;
    
    
    if(datatable==NULL)
        rt=all_tables;
    else
        rt=datatable;
    
    for (tm=rt->next;tm!=rt;tm=tm->next) 
        size+=ll_tablesize(tm->id);
    
    return size;
    
}

float ll_idTime() {

	//printf("[[%f]]\n",insert_delete_time);

	return (float)insert_delete_time;

}

float ll_sTime() { 

	return (float)search_time;

}

void * ll_fromary_time(void *data) {

    float *t=malloc(sizeof(float));
    
    struct ary_table_meta *ary_tab;
    
    int max_tables;
    
    ary_tab=(struct ary_table_meta *)(data);
    
    max_tables=ary_tab[0].notables;
    
    *t=(float)max_tables;
    
    return t;
}

int ll_maxindex(int tid, int *max_x, int *max_y) {
    
    struct ll_table_meta *tm;
    struct ll_table_ele *t,*tc,*tr;
    
    *max_x=*max_y=-1;
    
    tm=find_ll_table_meta(tid);
    if(tm==NULL)
        return NO_SUCH_TABLE;
    
    t=tm->table;
    if(t==NULL) 
        return OK;
    
    for(tc=t->rnext;tc!=t;tc=tc->rnext) {
        *max_y=max(*max_y,tc->y);
        for(tr=tc->lnext;tr!=tc;tr=tr->lnext)
            *max_x=max(*max_x,tr->x);
        
    }
    
    return OK;
    
}

int ll_iterate_tid(int flag) {
    
    static struct ll_table_meta *i;
    struct ll_table_meta *j;
    
    if(flag==0) {
        i=all_tables->next;
    }
    
    
    for (j=i;j!=all_tables;j=j->next)
        if(j->id !=-1)
            break;
    if(j==all_tables) {
        i=all_tables->next;
        return -1;
    }
    else {
        //if(j->next==all_tables)
        //	i=all_tables->next;
        //else 
        i=j->next;
        return j->id;
        
    }
}


#ifdef TEST


int main(void) {
    
    int t;
    int e,p;
    int x,y;
    int i,j;
    int *k;
    
//int z;
    
    ll_init(NULL);
    e=3;
    p=20;
    t=ll_newtable();
    
    i=ll_iterate_tid(0);
    j=ll_iterate_tid(1);
    
    printf ( "[[%d, %d]]\n",i,j);
    ll_insert(t,3,5,(void *)&e);
    ll_insert(t,1,1,(void *)&e);
    ll_insert(t,10,300,(void *)&p);
    ll_insert(t,1000,3000,(void *)&e);
    
    ll_maxindex(t,&x,&y);
    printf("max=(%d,%d)\n",x,y);
    
    
    for(i=0;i<=x;i++)
        for(j=0;j<=y;j++)
            if ((k=ll_elementAt(t,i,j))!=NULL)
                printf("(%d,%d)=%d\n",i,j,*k);
    
    printf("memUsed=%d\n",ll_memUsed(NULL));
    
    ll_delete(t,1,1);
    
    printf("memUsed=%d\n",ll_memUsed(NULL));
    ll_delete(t,3,5);
//ll_delete(t,10,300);
    
    printf("memUsed=%d\n",ll_memUsed(NULL));
    ll_delete(t,1000,3000);
    
    printf("memUsed=%d\n",ll_memUsed(NULL));
    
    ll_insert(t,30,200,(void *)&p);
    
    ll_search(t,&p,sizeof(int),&x,&y);
    printf("x=%d,y=%d\n",x,y);
    ll_search(t,NULL,sizeof(int),&x,&y);
    printf("x=%d,y=%d\n",x,y);
    printf("elementAt(%d,%d)=%d\n",x,y,*(int *)ll_elementAt(t,x,y));
    print_table(NULL);
    
    ll_freetable(t);
    print_table(NULL);
    
    t=ll_newtable();
    ll_insert(t,56,34,(void *)&e);
    print_table(NULL);
    
    return 0;
}

#endif
