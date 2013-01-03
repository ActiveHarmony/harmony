/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
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
#ifndef _LIBLL_H
#define _LIBLL_H

struct ll_table_ele {

        int x;
        int y;
        void *ele;
        struct ll_table_ele *lprev;
        struct ll_table_ele *lnext;
        struct ll_table_ele *rprev;
        struct ll_table_ele *rnext;
};

struct ll_table_meta{

        int id;
        struct ll_table_ele *table;
        struct ll_table_meta *next;
        struct ll_table_meta *prev;
} ;

void ll_init(struct ll_table_meta **data);
int ll_newtable();
int ll_insert(int tid, int x, int y, void *e);
int ll_delete(int tid, int x, int y);
int ll_search(int tid, void *e, int size, int *x, int *y);
void * ll_elementAt(int tid, int x, int y);
int ll_freetable(int tid);
void ll_fromary(void **data);
int ll_memUsed(void *datatable);

#endif /* _LIBLL_H */


