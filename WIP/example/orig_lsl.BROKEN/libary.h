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
