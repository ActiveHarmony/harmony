
/*
 * $Revision: 1.1.1.1 $
 * $Id: queue2.h,v 1.1.1.1 2001/11/09 20:46:15 ihchung Exp $
 */

#ifndef _QUEUE2_H
#define _QUEUE2_H

#define getmem malloc

		
/**** Some list manipulation macros. Next, Insert, Delete ****/
#define InitDQ(head,type) { \
    head = (type *) getmem(sizeof(type)) ;\
    head->next = head->prev = head ;\
}

#define NextDQ(ptr)(ptr->next)


#define InsertDQ(pos, element){ \
    element->next = pos->next; \
    pos->next->prev = element; \
    element->prev = pos; \
    pos->next = element ; \
}


#define InsertDQ_l(pos, element){ \
    element->lnext = pos->lnext; \
    pos->lnext->lprev = element; \
    element->lprev = pos; \
    pos->lnext = element ; \
}

#define InsertDQ_r(pos, element){ \
    element->rnext = pos->rnext; \
    pos->rnext->rprev = element; \
    element->rprev = pos; \
    pos->rnext = element ; \
}


/* DOESN'T FREE */
#define DelNextDQ(pos){\
    pos->next->next->prev = pos;\
    pos->next = pos->next->next;\
}


#define EmptyDQ(h)(h->next==h)

#define EmptyDQ_l(h)(h->lnext==h)
#define EmptyDQ_r(h)(h->rnext==h)
#define DelDQ(pos){ \
    pos->prev->next = pos->next;\
    pos->next->prev = pos->prev;\
}


#define DelDQ_l(pos){ \
    pos->lprev->lnext = pos->lnext;\
    pos->lnext->lprev = pos->lprev;\
}

#define DelDQ_r(pos){ \
    pos->rprev->rnext = pos->rnext;\
    pos->rnext->rprev = pos->rprev;\
}




/* Generic singly linked (non circular) queue ----------------------------*/
typedef struct GenericStruct {
  struct GenericStruct * next ;
} GenericList ; /* Generic List
		 * used for list manipulation functions. 
		 * All lists should
		 * have 'next' as their first field 
		 */
		


/**** Some list manipulation macros. Next, Insert, Delete ****/
#define InitQ(head,type) { \
    head = (type *) getmem(sizeof(type)) ;\
    head->next = NULL ;\
}

#define NextQ(ptr)(ptr->next)

#define InsertQ(pos, element){ \
    element->next = pos->next; \
    pos->next = element ; \
}

/* DOESN'T FREE */
#define DelNextQ(pos){\
    pos->next = pos->next->next;\
}


#define EmptyQ(h)(h->next==NULL)

#endif

