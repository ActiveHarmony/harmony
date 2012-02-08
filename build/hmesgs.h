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

/*
 * included header files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>


#include "hutil.h"
#include <vector>
#include <list>
#include <algorithm>

using namespace std;


#ifndef __HMESGS_H__
#define __HMESGS_H__

/***
 *
 * Define the different message types
 *
 ***/

/* no type */
#define HMESG_NONE 0
/* node description */
#define HMESG_NODE_DESCR 1
/* application description */
#define HMESG_APP_DESCR 2
/* daemon registration */
#define HMESG_DAEMON_REG 3
/* client registration */
#define HMESG_CLIENT_REG 4
/* variable registration */
#define HMESG_VAR_DESCR 5
/* variable value request */
#define HMESG_VAR_REQ 6
/* variable value set */
#define HMESG_VAR_SET 7
/* unregister client */
#define HMESG_CLIENT_UNREG 8
/* confirmation message needed for synchronization */
#define HMESG_CONFIRM 9
/* failure on server side */
#define HMESG_FAIL 10
/* the update of the performance metric. (observed Goodness) */
#define HMESG_PERF_UPDT 11
/* request probe/local tcl variables */
#define HMESG_PROBE_REQ 12
#define HMESG_TCLVAR_REQ 13
#define HMESG_TCLVAR_REQ_2 14
#define HMESG_PROBE_SET 15
#define HMESG_DATABASE 16
#define HMESG_WITH_CONF 17
#define HMESG_CODE_COMPLETION 18
#define HMESG_PERF_ALREADY_EVALUATED 19
#define HMESG_CFG_REQ 20

/*
 * define accepted variable types
 */
#define VAR_UNKNOWN -1
#define VAR_INT      0
#define VAR_STR      1
#define VAR_REAL     2

/*
 * define the maximum size of a string variable
 */
#define VAR_STR_SIZE 4096

/***
 *
 * type definitions
 *
 ***/

// the definition of the class VarDef
// that keeps information about the variables
// that are registered with Harmony
class VarDef {
  private:
    int type;          // Variable type.
    char *name;        // Variable name.
    int is_internal;   // Flag to specify if ptr was allocated internally.
    void *ptr;         // Pointer to internal or external value.
    void *shadow;      // Pointer to internal shadow value.
    unsigned int size; // Length of data.

  public:
    // the default constructor
    VarDef()
    {
        type = VAR_UNKNOWN;
        name = NULL;
        is_internal = 1;
        ptr = NULL;
        shadow = NULL;
    }

    // constructor that takes as params the variable name and its type
    VarDef(const char *vN, int vT)
    {
        type = vT;
        if (vN) {
            name = (char *)malloc(strlen(vN) + 1);
            strcpy(name, vN);
        }
        else {
            name = NULL;
        }

        is_internal = 1;
        ptr = NULL;
        shadow = NULL;
    }

    // this method is local and will only be used to allocate memory
    // for the variables. It is a conveninent way to reuse code
    void alloc()
    {
        switch (getType()) {
        case VAR_UNKNOWN:
            assert(0 && "Invalid VarDef type for alloc().");
            break;
        case VAR_INT:
            if (ptr == NULL && is_internal) {
                ptr = malloc(sizeof(int));
            }
            if (shadow == NULL) {
                shadow = malloc(sizeof(int));
            }
            size = sizeof(int);
            break;
        case VAR_STR:
            break;
        case VAR_REAL:
            if (ptr == NULL && is_internal) {
                ptr = malloc(sizeof(double));
            }
            if (shadow == NULL) {
                shadow = malloc(sizeof(double));
            }
            size = sizeof(double);
            break;
        }
        is_internal = 1;
    }

    // copy constructor
    VarDef(const VarDef& v)
    {
        type = v.type;

        if (v.name) {
            name = (char *)malloc(strlen(v.name)+1);
            strcpy(name, v.name);
        } else {
            name = NULL;
        }

        is_internal = v.is_internal;

        if (v.ptr) {
            switch (type) {
            case VAR_INT:
                if (is_internal) {
                    ptr = malloc(sizeof(int));
                    *((int *)ptr) = *((int *)v.ptr);
                }
                else {
                    ptr = v.ptr;
                }
                shadow = malloc(sizeof(int));
                *((int *)shadow) = *((int *)ptr);
                size = sizeof(int);
                break;
            case VAR_STR:
                assert(strlen((char *)v.ptr) < VAR_STR_SIZE);
                if (is_internal) {
                    ptr = malloc(VAR_STR_SIZE);
                    strcpy((char *)ptr, (char *)v.ptr);
                }
                else {
                    ptr = v.ptr;
                }
                shadow = malloc(VAR_STR_SIZE);
                strcpy((char *)shadow, (char *)ptr);
                size = VAR_STR_SIZE;
                break;
            case VAR_REAL:
                if (is_internal) {
                    ptr = malloc(sizeof(double));
                    *((double *)ptr) = *((double *)v.ptr);
                }
                else {
                    ptr = v.ptr;
                }
                shadow = malloc(sizeof(double));
                *((double *)shadow) = *((double *)ptr);
                size = sizeof(double);
                break;
            }
        } else {
            ptr = NULL;
            shadow = NULL;
        }
    }

    // destructor
    ~VarDef()
    {
        if (name) free(name);
        if (ptr && is_internal) free(ptr);
        if (shadow) free(shadow);
    }

    // the equal operator
    // it might be useful later
    VarDef &operator=(const VarDef &v)
    {
        if (this != &v) {
            setName(v.name);
            type = v.type;
            size = v.size;
            is_internal = v.is_internal;
            if (!is_internal) {
                ptr = v.ptr;
            }
            else if (v.ptr) {
                ptr = malloc(size);
                memcpy(ptr, v.ptr, size);
            }
            else {
                ptr = NULL;
            }

            if (v.shadow) {
                shadow = malloc(size);
                memcpy(shadow, v.shadow, size);
            }
            else {
                shadow = NULL;
            }
        }
        return *this;
    }

    // set the value of the variable
    // if the variable is an int
    inline void setValue(int v)
    {
        if (type == VAR_INT) {
            if (ptr == NULL)
                ptr = malloc(sizeof(int));
            *((int *)ptr) = v;

            setShadow(v);
        }
    }

    // set the shadow of the variable
    // if the variable is an int
    inline void setShadow(int v)
    {
        if (type == VAR_INT) {
            if (shadow == NULL)
                shadow = malloc(sizeof(int));
            *((int *)shadow) = v;
        }
    }

    // set the value of the variable
    // if the variable is a string
    inline void setValue(const char *s)
    {
        if (type == VAR_STR) {
            if (ptr == NULL)
                ptr = malloc(VAR_STR_SIZE);
            assert(strlen(s) < VAR_STR_SIZE);
            strcpy((char *)ptr, s);

            setShadow(s);
        }
    }

    // set the shadow of the variable
    // if the variable is a string 
   inline void setShadow(const char *s)
   {
        if (type == VAR_STR) {
            if (shadow == NULL)
                shadow = malloc(VAR_STR_SIZE);

            assert(strlen(s) < VAR_STR_SIZE);
            strcpy((char *)shadow, s);
        }
    }

    // set the value of the variable
    // if the variable is a real number
    inline void setValue(double v)
    {
        if (type == VAR_REAL) {
            if (ptr == NULL)
                ptr = malloc(sizeof(double));
            *((double *)ptr) = v;

            setShadow(v);
        }
    }

    // set the shadow of the variable
    // if the variable is a real number
    inline void setShadow(double v)
    {
        if (type == VAR_REAL) {
            if (shadow == NULL)
                shadow = malloc(sizeof(double));
            *((double *)shadow) = v;
        }
    }

    // return the pointer to the value of the variable
    inline void *getPointer()
    {
        return ptr;
    }

    // return the pointer to the value of the variable
    inline void setPointer(void *external_ptr)
    {
        ptr = external_ptr;
        is_internal = 0;
    }

    // return the pointer to the shadow of the variable
    inline void *getShadow()
    {
        return shadow;
    }

    // return the type of the variable
    int getType()
    {
        return type;
    }

    // set the type of the variable
    inline void setType(int vt)
    {
        type = vt;
    }

    // get the name of the variable
    inline char *getName()
    {
        return name;
    }

    // set the name of the variable
    inline void setName(const char *vN)
    {
        int len = 0;
        if (vN != NULL) {
            len = strlen(vN) + 1;
        }

        name = (char *)realloc(name, len);
        strncpy(name, vN, len);
    }

    // serialize the variable content
    int serialize(char *);

    // deserialize the variable content
    int deserialize(char *);

    // print the variable content
    // for debugging purposes only
    void print();
};

/***
 *
 * define the base message class
 *
 ***/

static const char *print_type[] = {"NONE",
                                   "NODE_DESCR",
                                   "APP_DESCR",
                                   "DAEMON_REG",
                                   "CLIENT_REG",
				   "CODE_GEN_REG",
                                   "VAR_DESCR",
                                   "VAR_REQ",
                                   "VAR_SET",
                                   "CLIENT_UNREG",
                                   "CONFIRM",
                                   "FAIL",
                                   "PERF_UPDT",
                                   "PROBE_REQ",
                                   "TCLVAR_REQ",
                                   "TCLVAR_REQ_2",
                                   "PROBE_SET",
                                   "DATABASE",
                                   "WITH_CONF",
                                   "CODE_COMPLETION",
                                   "PERF_ALREADY_EVALUATED",
                                   "CFG_REQ"};

// the general message class
class HMessage {
  private:
    /* the version of the protocol */
    unsigned long int version;

    /* the message type kept in network byte order */
    unsigned long int type;
    
    /* parameter that specifies whether the variable is a flag/local
       variable
     */
    //unsigned long int flag;

  public:
    // default constructor
    HMessage() {
        type=htonl(HMESG_NONE);
        version=htonl(2);
        //flag = htonl(0);
    }

    // constructor that takes message type as parameter
    HMessage(int t) {
        type=htonl(t);
        version=htonl(2);
        //flag=htonl(0);
    }

    HMessage(int t, int f) {
        type=htonl(t);
        version=htonl(2);
        //flag = htonl(f);
    }


    // default destructor
    ~HMessage() {
    }

    // set type of the message
    inline void set_type(int t) {
        type=htonl(t);
    }

    // get the type of the message
    inline int get_type() {
        return ntohl(type);
    }

    // get the version of the protocol
    inline int get_version() {
        return ntohl(version);
    }

    // return the size of the message in bytes
    virtual int get_message_size() {
        //return 2*sizeof(unsigned long int);
        return 2*sizeof(unsigned long int);
    }

    // serialize message
    virtual int serialize(char *buf);

    // deserialize message
    virtual int deserialize(char *buf);

    // print content of message
    // for debug purposes only
    virtual void print();
};


/**
 *
 * define child classes
 *
 **/
// the Registration Message class
// is a child class of the Message class
// and defines a field param which has different
// interpretations depending on when the message
// is sent
// for example, when the first registration message
// is sent to the server the param has the value of
// the global client variable huse_signals
// identification is an id that allows application to migrate
// and reregister (get a new socket on the server) under the old name
class HRegistMessage : public HMessage {
  private:
    unsigned long int param;
    int identification;

  public:
    // default constructor
    HRegistMessage():HMessage() {
        identification = param=htonl(0);
    }

    // constructor that takes message type and param value
    HRegistMessage(int t, int p):HMessage(t) {
        param=htonl(p);
        identification = htonl(0);
    }

    // return the value of the param
    inline int get_param() {
        return ntohl(param);
    }

    // set the value of the param
    inline void set_param(int p) {
        param=htonl(p);
    }

    // return the value of the identification
    inline int get_id() {
        return ntohl(identification);
    }

    // set the value of the identification
    inline void set_id(int p) {
        identification=htonl(p);
    }

    // return the size of the message in bytes
    int get_message_size() {
        return (HMessage::get_message_size()+sizeof(unsigned long int)+sizeof(int));
    }

    // serialize the message
    int serialize(char *buf);

    // deserialize the message
    int deserialize(char *buf);

    // print message content
    // for debug purposes
    void print();
};

// the Update Message is used when setting or requesting
// variables from the server
class HUpdateMessage : public HMessage {
  private:
    // number of variables in the message
    unsigned long int nr_vars;

    // timestamp used by the Simplex minimization method
    unsigned long int timestamp;

    // list of variables (as a vector!)
    vector<VarDef> var_list;

  public:
    // default constructor initializes nr of variables to 0
    HUpdateMessage():HMessage() {
        nr_vars=htonl(0);
        timestamp=0;
    }

    // constructor that takes message type and nr_vars
    // it should not be used with a value for nr_vars other than 0
    // since set_var increments nr_vars
    HUpdateMessage(int t, int nr): HMessage(t) {
        nr_vars=htonl(nr);
        timestamp=0;
    }

    // return the number of variables in the message
    inline int get_nr_vars() {
        return ntohl(nr_vars);
    }

    // return the n-th variable in the message
    VarDef *get_var(int n);


    // set the number of variables in the message to a given value
    inline void set_nr_vars(int n) {
        nr_vars=htonl(n);
    }

    // adds a variable to the message
    // as a side-effect it increments the number of variables
    void set_var(VarDef v);

    // set the timestamp
    inline void set_timestamp(unsigned long int t) {
        timestamp=htonl(t);
    }

    // get the timestamp
    unsigned long int get_timestamp() {
        return ntohl(timestamp);
    }

    // return the size of the message (in bytes)
    int get_message_size();

    // serialize the message
    int serialize(char *buf);

    // deserialize the message
    int deserialize(char *buf);

    // print message info
    // for debug purposes
    void print();
};

// this class carries descriptions along the net
// the purpose of defining the Description message class is
// to be used when sending application descriptions and variable registration
class HDescrMessage : public HMessage {
  private:
    // the description that is carried and its length
    unsigned short int descrlen;
    char *descr;

  public:
    // default constructor
    HDescrMessage():HMessage(){
        descr=NULL;
        descrlen=htonl(0);
    }

    // constructor given type of message, description and description length
    HDescrMessage(int t, const char *d, int l):HMessage(t){
        descrlen = htons(l);
        descr = (char *)malloc(sizeof(char) * (l+1));
        strncpy(descr, d, l);
        descr[l] = '\0';
    }

    // destructor
    ~HDescrMessage() {
        //    delete[] descr;
        free(descr);
    }

    // return description
    inline char *get_descr() {
        return descr;
    }

    // set description inside the message given a string and its length
    inline void set_descr(const char *d, int l) {
        free(descr);
        set_descr(d,l,0);
    }

    // set description inside the message given a string and its length
    // but with the option of freeing the old one first.
    inline void set_descr(const char *d, int l, int erase) {
        if (!erase) {
            descrlen=htons(l);
            //descr=new char[l+1];
            descr = (char *)malloc(sizeof(char)*(l+1));
            strcpy(descr,d);
        }
        else {
            free(descr);
            descrlen=htons(l);
            descr = (char *)malloc(sizeof(char)*(l+1));
            //      descr=new char[l+1];
            strcpy(descr,d);
        }
    }


    // return the description length
    inline int get_descrlen() {
        return ntohs(descrlen);
    }


    // return the message size (in bytes)
    int get_message_size() {
        return HMessage::get_message_size()+get_descrlen()+sizeof(unsigned long int);
    }

    // serialize message
    int serialize(char *buf);

    // deserialize message
    int deserialize(char *buf);

    // print content of message
    // for debug purposes
    void print();

};

#endif  /* ifndef __HMESGS_H__ */
