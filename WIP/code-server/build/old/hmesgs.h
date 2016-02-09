/*
 * Copyright 2003-2016 Jeffrey K. Hollingsworth
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

#include <vector>
#include <list>
#include <algorithm>
#include <assert.h>

#include "hutil.h"

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
#define  HMESG_PROBE_REQ 12

#define  HMESG_TCLVAR_REQ 14

#define HMESG_PROBE_SET 13

#define HMESG_DATABASE 15

#define HMESG_WITH_CONF 16

#define HMESG_PROJECTION 17

#define HMESG_SIM_CONS_REQ 18

#define HMESG_SIM_CONS_RESULT 19

#define HMESG_PROJ_REQ 20

#define HMESG_PROJ_RESULT 21
/*
 * define accepted variable types
 */
#define VAR_INT 0
#define VAR_STR 1

/*
 * define the maximum size of a string variable
 * for now we only define it 2048 chars but we might increase it in time
 */
#define VAR_STR_SIZE 2048

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
    // the name of the variable
    char *varName;
    // the variable type: VAR_INT or VAR_STR
    unsigned long int varType;
    // the pointer to the value of the variable
    void *varp;
    // the pointer to the shadow value of the variable
    void *shadowp;
    // the size of variable (string length)

    //the original value
    // note that this is used when some of the processors
    // do not participate in optimizing the values for some
    // of the variables. This attribute simply stores the
    // original value of the variable: the value before
    // any of the optimizations are performed.
    //void *origVal;

  public:

    // the default constructor
    VarDef() {
        varName=NULL;
        varType=htonl(5);
        varp=NULL;
        shadowp=NULL;
        //origVal=NULL;
    }

    // constructor that takes as params the variable name and its type
    VarDef(char *vN, int vT) {
        varName=(char *)malloc(strlen(vN));
        strcpy(varName,vN);
        varType=htonl(vT);
        varp=NULL;
        shadowp=NULL;
        // Note that the
        //origVal=NULL;
    }

    // this method is local and will only be used to allocate memory
    // for the variables. It is a conveninent way to reuse code
    void alloc (){
        switch (getType()) {
            case VAR_INT:
                varp=(int *)malloc(sizeof(int));
                shadowp=(int *)malloc(sizeof(int));
                //origVal=(int *)malloc(sizeof(int));
                break;
            case VAR_STR:
                printf("check point alloc: hmesgs.h:\n");
                varp = (char*)malloc(VAR_STR_SIZE);
                shadowp = (char*)malloc(VAR_STR_SIZE);
                //origVal = (char*)malloc(VAR_STR_SIZE);
                *(char*)varp = '\0';
                *(char*)shadowp = '\0';
                // *(char*)origVal = '\0';
                break;
        }
    }



    // copy constructor
    VarDef(const VarDef& v) {

        //printf("vardef varName %s varNameLength %d\n", (char*)v.varName,strlen(v.varName));

        varName=(char *)malloc(strlen(v.varName));
        strcpy(varName,v.varName);

        varType=v.varType;

        switch (getType()) {
            case VAR_INT:
                varp=(int *)malloc(sizeof(int));
                *((int *)varp)=*((int *)v.varp);
                shadowp=(int *)malloc(sizeof(int));
                *((int *)shadowp)=*((int *)v.varp);
                //origVal=(int *)malloc(sizeof(int));
                //*((int *)origVal)=*((int *)v.origVal);
                break;
            case VAR_STR:
                assert(strlen((char *)v.varp) < VAR_STR_SIZE);
                varp = (char*)malloc(VAR_STR_SIZE);
                strcpy((char *)varp,(char *)v.varp);
                shadowp = (char*)malloc(VAR_STR_SIZE);
                strcpy((char *)shadowp,(char *)v.varp);
                //origVal = (char*)malloc(VAR_STR_SIZE);
                //strcpy((char *)origVal,(char *)v.origVal);
                break;
        }

    }


    // destructor
    ~VarDef() {
        //printf("vardef deconstruction? \n");
        free(varName);
        free(varp);
        free(shadowp);
        //free(origVal);
    }


    // the equal operator
    // it might be useful later
    VarDef& operator=(const VarDef &v) {

        if (this!= &v) {

            setName(v.varName);

            setType(ntohl(v.varType));

            switch (getType()) {
                case VAR_INT:
                    setValue(*((int *)v.varp));
                    break;
                case VAR_STR:
                    setValue((char *)v.varp);
                    break;
            }
        }
        return *this;
    }



    // set the value of the variable
    // if the variable is an int
    inline void setValue(int v){
        if (getType()==VAR_INT) {
            if (varp == NULL)
                varp = (int*)malloc(sizeof(int));
            *((int *)varp)=v;

            setShadow(*(int *)varp);
        }

    }


    // set the shadow of the variable
    // if the variable is an int
    inline void setShadow(int v){
        if (getType()==VAR_INT) {
            if (shadowp == NULL)
                shadowp = (int*)malloc(sizeof(int));

            *((int *)shadowp)=v;
        }
    }

/*
     inline void setOrig(int v){
        if (getType()==VAR_INT) {
            if (origVal == NULL)
                origVal = (int*)malloc(sizeof(int));
            *((int *)origVal)=v;
        }
    }
*/
    // set the value of the variable
    // if the variable is a string
    inline void setValue(char *s) {
        if (getType()==VAR_STR) {
            if (varp == NULL)
                varp = (char*)malloc(VAR_STR_SIZE);
            assert (strlen(s) < VAR_STR_SIZE);
            strcpy((char *)varp,s);
            setShadow((char *)varp);
        }
    }

    // set the shadow of the variable
    // if the variable is a string
   inline void setShadow(char *s) {
        if (getType()==VAR_STR) {
            if (shadowp == NULL)
                shadowp = (char*)malloc(VAR_STR_SIZE);

            assert (strlen(s) < VAR_STR_SIZE);
            strcpy((char *)shadowp,s);
        }
    }
/*
    inline void setOrig(char *s) {
        if (getType()==VAR_STR) {
            if (origVal == NULL)
                origVal = (char*)malloc(VAR_STR_SIZE);

            assert (strlen(s) < VAR_STR_SIZE);
            strcpy((char *)origVal,s);
        }
    }
*/
    // return the pointer to the value of the variable
    inline void *getPointer()
    {
        return varp;
    }

    // return the pointer to the shadow of the variable
    inline void *getShadow()
    {
        return shadowp;
    }
/*
    inline void *getOrig()
    {
        return origVal;
    }
*/
    // return the type of the variable
    int getType()
    {
        return ntohl(varType);
    }


    // set the type of the variable
    inline void setType(const int vt)
    {
        varType=htonl(vt);
    }


    // get the name of the variable
    inline char *getName()
    {
        return varName;
    }

    // set the name of the variable
    inline void setName(const char *vN)
    {
        varName=(char *)realloc(varName, strlen(vN));
        strcpy(varName,vN);
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
                                   "VAR_DESCR",
                                   "VAR_REQ",
                                   "VAR_SET",
                                   "CLIENT_UNREG",
                                   "CONFIRMATION",
                                   "FAILURE",
                                   "PERFORMANCE_UPDT",
                                   "PROBE_REQ",
                                   "TCL_VAR_REQ",
                                   "PROBE_SET",
                                   "DATABASE",
                                   "WITH_CONF"};


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

/*
    inline void set_flag(int f)
    {
        flag = htonl(f);
    }

    inline int get_flag()
    {
        return ntohl(flag);
    }
*/

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
    HDescrMessage(int t, char *d, int l):HMessage(t){
        descrlen=htons(l);
        //    descr=new char[l+1];
        descr = (char *)malloc(sizeof(char)*(l+1));
        strcpy(descr,d);
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
    inline void set_descr(char *d, int l) {
        free(descr);
        set_descr(d,l,0);
    }

    // set description inside the message given a string and its length
    // but with the option of freeing the old one first.
    inline void set_descr(char *d, int l, int erase) {
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


#endif  /* ifndef __HMESGS_H_ */
