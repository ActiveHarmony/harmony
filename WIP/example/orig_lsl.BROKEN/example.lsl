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

/* $Revision: 1.11 $ */
%library TwoDTable

	#include "localresource.h"

%interface c

void init();
int newtable();
int insert(int tid, int x, int y, void *e);
int delete(int tid, int x, int y);
int search(int tid, void *e, int size, int *x, int *y);
void *elementAt(int tid, int x, int y);
int freetable(int tid);


%variable

	/* remote variables with default value */
	/* variable name starting with '@' can be set by requester */
	int @sparse=1; 
	int @search_often=0;
	int @large_index_range=1;
	int @datasize=400;

	/* variable name starting with '$' should be binded to resource management */
	int mem=MEMORYSIZE;

	/* other variables */
	void *data;  /* data structure to hold all data in the table */

%metric  
/* the order should match with the estimation/measurement part of each
casein the method */

	float insert_delete_time;
	int memory;           /* datatype, name */
	float search_time;

%method
	LINKLIST {
		filename libll.so
		function {

			init(): ll_init(&data);
			newtable(): ll_newtable();
			insert(): ll_insert(tid, x, y, e);
			delete(): ll_delete(tid, x, y);
			search(): ll_search(tid, e, size, x, y);
			elementAt(): ll_elementAt(tid, x, y);
			freetable():  ll_freetable(tid);
		}
		estimation {
			/* the parameters used for estimation
			   should from remote variables and
			   local resource variables only */
			memory: ll_EstimateMem(datasize);
			insert_delete_time: ll_EstimateID();
			search_time: ll_EstimateS();	
		}
		measurement {
			memory : ll_memUsed(data);
			insert_delete_time: ll_idTime();
			search_time: ll_sTime();
		}
		convertfrom {

			/* return type has to be void or int */
			ARRAY : void ll_fromary(&data);
			
		}
		convertfrom_est {

			/* return type has to be void* */
			ARRAY: ll_fromary_time(data);
		}
	}
	ARRAY {
		filename libary.so
		function {
	
			init(): ary_init(&data);
			newtable(): ary_newtable();
			insert(): ary_insert(tid, x, y, e);
			delete(): ary_delete(tid, x, y);
			search(): ary_search(tid, e, size, x, y);
			elementAt(): ary_elementAt(tid, x, y);
			freetable(): ary_freetable(tid);
		}
                estimation {
			memory: ary_EstimateMem(datasize);
			insert_delete_time: ary_EstimateID();
			search_time: ary_EstimateS();
		}
                measurement {
			memory: ary_memUsed(data);
			insert_delete_time: ary_idTime();
			search_time: ary_sTime();
		}
		convertfrom {
			LINKLIST: void ary_fromll(&data);
		}		
		convertfrom_est {
			LINKLIST: ary_fromll_time(data);
		}

        }


%rule 

	predicate {
		IsSparse: sparse==1
		NotFastSearch: search_often==0
		WideIndex: large_index_range==1
		largedata: datasize > mem
	}
	truthtable {
		condition (IsSparse) : LINKLIST
		/* condition (!NotFastSearch && !WideIndex) : ARRAY
		condition (largedata) : LINKLIST */
	}
	decision
		automatic
		/* manual or automatic; default: manual */
		/* automatic will do the conversion if necessary */

