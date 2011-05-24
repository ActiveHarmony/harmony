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

/***
 *
 * This header file contains the type definitions of some of the objects
 * used to store the harmony database. That is, the information about 
 * daemons, nodes, clients, variables, etc.
 *
 ***/

#ifndef __HDB_H__
#define __HDB_H__

#include<string.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>




class Hnode{
 public:
  Hnode(struct sockaddr addr) {
    address=addr;
    description=NULL;
  }

  Hnode(Hnode *otherNode) {
    address=otherNode->get_address();
    set_description(otherNode->get_description());
  }

  ~Hnode() {
    free(description);
  }

  inline struct sockaddr get_address() { return address; }
  inline void set_address(struct sockaddr addr) {address=addr;}
  inline void set_description(char *descr) {
    description=(char *)malloc(strlen(descr)*sizeof(char));
    strcpy(description, descr);
  }
  inline char * get_description() {return description;}


 private:
  struct sockaddr address;

  char *description;
  
};



class HnodeElem {
 public:
  HnodeElem(Hnode *inf){
    info=new Hnode(inf);
    next=0;
  }

  ~HnodeElem() {
    delete info;
    if (next!=0) delete next;
  }
  
  inline void set_next(HnodeElem *n) { next=n; }
  inline HnodeElem *get_next() {return next;}
  inline void set_info(Hnode *n) {
 private:
  Hnode *info;
  HnodeElem *next;
};



class HnodeList {
 public:
  HnodeList() {
    head=0;
    tail=0;
  }

  ~HnodeList() {
    while (!this->empty()) {
      this->remove_head();
    }
  }

  inline void add_head(HnodeElem *elem) {
    HnodeElem temp = new HnodeElem()
    temp=new
 private:
  HnodeElem *head;
  HnodeElem *tail;
};



class Happlication {

 public:
  Happlication(struct sockaddr addr) {
    address=addr;
    description=NULL;
  }

  Happlication(Happlication *other_app) {
    address=other_app->get_address();
    set_address(other_app->get_address());
  }

  ~Happlication() {
    free(description);
  }

  inline struct sockaddr get_address() { return address; }
  inline void set_address(struct sockaddr addr) {address=addr;}
  inline void set_description(char *descr) {
    description=(char *)malloc(strlen(descr));
    strcpy(description, descr);
  }
  inline char * get_description() {return description;}

 private:
  struct sockaddr address;

  char *description;
  
};


#endif /* __HDB_H__ */




