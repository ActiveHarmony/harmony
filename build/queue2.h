/*
 * Copyright 2003-2012 Jeffrey K. Hollingsworth
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

