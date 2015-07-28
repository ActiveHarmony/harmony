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

/* $Revision: 1.1 $ */

 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/times.h>
 #include <sys/time.h>
 #include <sys/resource.h>
 #include <unistd.h>
 #ifdef LINUX
 #include <hrtime.h>
 #endif

#ifdef _LSL_
 #include "TwoDTable.h"
#endif

#ifdef _LIBLL_
 #include "libll.h"
#endif 

#ifdef _LIBARY_
 #include "libary.h"
#endif

/* #define NOTEST 5000 */
 #define MAX_X 5000
 #define MAX_Y 5000

 #define Irate 0.5
 #define Drate 0.5
 #define Srate 0

 #define Rsearch .5
 #define Nsearch .5
 

 struct ele {
	int number;
	int x;
	int y;
 };

float elapsed_time;

void measure_time(float *result) {
/*
	static struct tms t1,t2;

	if (result==NULL)
		times(&t1);
	else {
		times(&t2);
		*result=(float)(t2.tms_utime-t1.tms_utime+t2.tms_stime-t1.tms_stime);
	}

*/

#ifdef LINUX

        static int first=1;
        static hrtime_t c1,c2;
        static struct hrtime_struct *hr;
        int error;

        if(first==1) {

                first=0;

                error = hrtime_init();
                if (error < 0)
                {
                        printf("Error: hrtime_init()\n");
                        exit(1);
                }

                error = get_hrtime_struct(0, &hr);
                if (error < 0)
                {
                        printf("Error: get_hrtime_struct()\n");
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
        if (result==NULL)
                t1=gethrvtime();
        else {
                t2=gethrvtime();
		*result=(float)((t2-t1)/1000000000.0);
	}
#endif

	return;
}
	


 int main(int argc, char **argv) {

	
	int NOTEST;

	struct ele *e;

	int i;
	int t;
	int *p=NULL;

	int *mem=malloc(sizeof(int));
	float *tm=malloc(sizeof(float));
	float total_time;

	//float total_mem=0;

	float i_rate=Irate/(Irate+Drate+Srate);
	float d_rate=Drate/(Irate+Drate+Srate)+i_rate;
	//float s_rate=Srate/(Irate+Drate+Srate);
	//float n_search= Nsearch/(Nsearch+Rsearch);
	float chance;

	//int res;

	//int f=5;

	//int x,y;
	int a,b,c;

	//struct rusage ru;

	a=b=c=0;

	//struct TwoDTable_metric *m;

	//struct tms t1,t2;

	if(argc!=2) {
		printf("[usage]try <number>\n");
		exit(1);
	}

        NOTEST=atoi(argv[1]);

        e=malloc(NOTEST*sizeof(struct ele));


	for(i=0;i<NOTEST;i++) {
		e[i].number=rand();
		e[i].x=1+(int) ((float)MAX_X*rand()/(RAND_MAX+1.0));
		e[i].y=1+(int) ((float)MAX_Y*rand()/(RAND_MAX+1.0));
	}


	//measure_time(NULL);

	
#ifdef _LSL_
	TwoDTable_init();
	init();
	t=newtable();
#endif

#ifdef _LIBLL_
	ll_init(NULL);
	t=ll_newtable();
#endif

#ifdef _LIBARY_
	ary_init(NULL);
	t=ary_newtable();
#endif


        //updateresult();

        //updateattribute(0,1,0,400);

	for(i=0;i<NOTEST;i++) {
#ifdef _LSL_
		insert(t,e[i].x,e[i].y,&(e[i].number));
#endif
#ifdef _LIBLL_
		ll_insert(t,e[i].x,e[i].y,&(e[i].number));
#endif
#ifdef _LIBARY_
		ary_insert(t,e[i].x,e[i].y,&(e[i].number));
#endif
	}

//printf("loading done\n");


	total_time=0;

	measure_time(NULL);
	for(i=0;i<NOTEST;i++) {
		chance=(float)(rand()/(RAND_MAX+1.0));
		if(chance <= i_rate) {
#ifdef _LSL_
        		insert(t,e[i].x,e[i].y,&(e[i].number));
#endif

#ifdef _LIBLL_
			ll_insert(t,e[i].x,e[i].y,&(e[i].number));
#endif

#ifdef _LIBARY_
			ary_insert(t,e[i].x,e[i].y,&(e[i].number));
#endif
			a++;

			//printf("insert\n");
		}
		else if(chance <= d_rate ) {
#ifdef _LSL_
			delete(t,e[i].x,e[i].y);
#endif

#ifdef _LIBLL_
			ll_delete(t,e[i].x,e[i].y);
#endif

#ifdef _LIBARY_
			ary_delete(t,e[i].x,e[i].y);
#endif
			b++;
			
			//printf("delete\n");
		}
		else {
			//chance=(float)(rand()/(RAND_MAX+1.0);
			//if(chance <= n_search)
#ifdef _LSL_
			//search(t,&(e[i].number),sizeof(int),&x,&y);
			p=(int *)elementAt(t,e[i].x,e[i].y);
#endif
                 
#ifdef _LIBLL_
			//ll_search(t,&(e[i].number),sizeof(int),&x,&y);
			p=(int *)ll_elementAt(t,e[i].x,e[i].y);
#endif

#ifdef _LIBARY_
			//ary_search(t,&(e[i].number),sizeof(int),&x,&y);
			p=(int *)ary_elementAt(t,e[i].x,e[i].y);
#endif
			c++;

			//printf("search\n");
		}
/*
#ifdef _LSL_
		//measurement("memory",&mem);
		//total_mem+=*mem;
#endif

#ifdef _LIBLL_
		total_mem+=ll_memUsed(NULL);
#endif

#ifdef _LIBARY_
		total_mem+=ary_memUsed(NULL);
#endif
*/


#ifdef _LSL_
		//measurement("memory",&mem);
		//printf("%d %d\n",i,*mem);
                //total_mem+=*mem;
		//measurement("insert_delete_time",&tm);
		//printf("%d %f\n",i,*tm);

		updateresult();

		//if(i==NOTEST/3) 
			updateattribute(0,1,0,400);
		//else if(i==NOTEST*2/3)
		//	updateattribute(1,1,1,5000);

		
#endif
		//printf("i=%d,(%d,%d,%d)\n",i,a,b,c);
	}

/*
		measurement("insert_delete_time",&tm);
		//updateresult();
		//updateattribute(i%2,1,0,400);
		//printf("%d %f\n",i,*tm);
		total_time+=*tm;
	}


	//printf("<<%ld %ld %ld %ld>>\n",t2.tms_utime,t1.tms_utime,t2.tms_stime,t1.tms_stime);

	printf("average insert time %f\n",total_time/i);

	//measurement("memory",&mem);

	//printf("mem used: %d\n",*mem);


	updateresult();

	updateattribute(0,1,0,400);

	printf("update done\n");

	total_time=0;
        for(i=NOTEST/2;i<NOTEST;i++) {
                insert(t,e[i].x,e[i].y,&(e[i].number));
		measurement("insert_delete_time",&tm);
                //printf("%d %f\n",i,*tm);
		total_time+=*tm;

        }


	printf("average insert time %f\n",total_time/(i-NOTEST/2));
	printf("\n");

        measurement("memory",&mem);

        printf("mem used: %d\n",*mem);


	total_time=0;
        for(i=0;i<NOTEST;i++) {
                res=delete(t,e[i].x,e[i].y);
		measurement("insert_delete_time",&tm);
                //printf("%d %f\n",i,*tm);
		total_time+=*tm;

        }
	//printf("average delete time %f\n",total_time/(i-1));
*/

	measure_time(&elapsed_time);

	printf("%d %f\n",NOTEST,elapsed_time);

//	printf("avg mem used=%f\n",total_mem/i);

	//printf("??%d??\n",getrusage(RUSAGE_SELF,&ru));
        //printf("((%ld,%ld,%ld,%ld))\n",ru.ru_maxrss,ru.ru_ixrss,ru.ru_idrss,ru.ru_isrss);

/*
	measurement("memory",&mem);
	printf("mem=%d\n",*mem);

	estimate("memory",NULL,&mem);
	printf("mem=%d\n",*mem);

        metricquery(&m);
        printf("[%s,%s]\n", m->name,m->datatype);


	
	p=(int *)elementAt(t,3,200);

	printf("p=%d\n",*p);

	setmethod("ARRAY");

	setmethod("LINKLIST");

	p=NULL;

        p=(int *)elementAt(t,3,200);

        printf("p=%d\n",*p);

	search(t,&f,sizeof(int),&x,&y);

	p=NULL;
	
	p=(int *)elementAt(t,x,y);

	printf("(%d,%d)=%d\n",x,y,*p);

	search(t,NULL,sizeof(int),&x,&y);

	p=NULL;

        p=(int *)elementAt(t,x,y);

        printf("(%d,%d)=%d\n",x,y,*p);
*/

	/* TODO: search ->convert -> search */
/*

	setmethod("ARRAY");

        search(t,&f,sizeof(int),&x,&y);

        p=NULL;

        p=(int *)elementAt(t,x,y);

        printf("(%d,%d)=%d\n",x,y,*p);

        search(t,NULL,sizeof(int),&x,&y);

        p=NULL;

        p=(int *)elementAt(t,x,y);

        printf("(%d,%d)=%d\n",x,y,*p);

	printf("[%s]\n",methodconversionquery("ARRAY"));

	if(methodconversionquery(NULL)==NULL)
		printf("yes\n");
*/

	free(mem);
	free(tm);
  return 0;
 }
