
/*
 * $Revision: 1.1 $
 * $Date: 2001/11/15 22:10:12 $
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


/* DOESN'T FREE */
#define DelNextDQ(pos){\
    pos->next->next->prev = pos;\
    pos->next = pos->next->next;\
}


#define EmptyDQ(h)(h->next==h)

#define DelDQ(pos){ \
    pos->prev->next = pos->next;\
    pos->next->prev = pos->prev;\
}





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

