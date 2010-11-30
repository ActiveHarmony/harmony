#ifndef _LIBARY_H
#define _LIBARY_H

struct ary_table_ele {

        void *ele;
        int max_x;

};

struct ary_table_meta {

        int id;
        int max_y;
        struct ary_table_ele *table;

	int notables; /* this field in the first element stores the
max_tables */

};
void ary_init(struct ary_table_meta **data);
int ary_newtable();
int ary_insert(int tid, int x, int y, void *e);
int ary_delete(int tid, int x, int y);
int ary_search(int tid, void *e, int size, int *x, int *y);
void * ary_elementAt(int tid, int x, int y);
int ary_freetable(int tid);
void ary_fromll(void **data);
int ary_memUsed(void *);





#endif
