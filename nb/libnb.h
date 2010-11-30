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

