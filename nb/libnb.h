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
#ifndef _LIBNB_H
#define _LIBNB_H

#define NB_OK 0;
#define NB_INDEX_ERR -100
#define NB_TARGET_ERR -101
#define NB_FUNC_ERR -102
#define NB_INIT_ERR -103
#define NB_FILE_ERR -104


#define NB_IGNORE_NULL 0
#define NB_SKIP_NULL 1
#define NB_ALL_ENTRY -1
#define NB_UNDECIDED -1


struct nb_result_ele {

        void * value;

};

struct nb_table_ele {

        struct nb_result_ele *result;
        int decision;
};

struct nb_table_ele *nb_table;
int no_attribute,no_tgt_value;
int (*nb_compare)(void *, void *);
void *(*nb_equation)(void *,void *);

int nb_init(int no_attr, int no_tgt, int (*comp)(void *,void *), void *(*update_equation)(void *, void *));
int nb_update(int index, int tgt, void * value);
int nb_suggest(int index);
int nb_undecide(int idx);
int nb_decide(int idx, int flag);
int nb_getindex(int no,...);
int nb_dumptable(char *filename,int (*dumpvoid)(FILE *,void *));
int nb_loadtable(char *filename, void *(*loadvoid)(FILE *));
int nb_setdecision(int index, int value);
void * nb_getvalue(int index, int tgt);
int print_nbtable(char *);


#endif

