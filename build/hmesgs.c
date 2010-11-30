
/******************************
 *
 * Author: Cristian Tapus
 * History:
 *   Jan 5, 2001   - comments and updated by Cristian Tapus
 *   Nov 20, 2000  - changes made by Djean Perkovic
 *   Sept 23, 2000 - comments added
 *   July 12, 2000 - first version
 *   July 15, 2000 - comments and update added by Cristian Tapus
 *
 *  Modified by: Ananta Tiwari
 *   Added origVal variable and its associated modifications to the functions.
 *******************************/


#include "hmesgs.h"

//using namespace std;

/***
 *
 * define functions used to pack, unpack message info
 *
 ***/

static int debug_mode;

// serialize the Variable definition
// this class is defined in the hmesgs.h file
int VarDef::serialize(char *buf) {
    int buflen=0;
    unsigned int temp;

    // first serialize the variable type, which is an int and can
    // be one of the following: VAR_INT or VAR_STR
    memcpy(buf+buflen, &varType, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);

    // then serialize the name of the variable.
    // when serializing strings put the length of the string first and
    // then the string itself
    temp=htonl(strlen(varName));
    memcpy(buf+buflen, &temp, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    memcpy(buf+buflen, varName, strlen(varName));
    buflen+=strlen(varName);

    // finally put the value of the variable into the buffer.
    // if the type is int, just add it to the buffer
    // if type is string, first add the length and then the string itself
    switch (getType()) {
        case VAR_INT:
            temp=htonl(*((int *)getPointer()));
            memcpy(buf+buflen, &temp, sizeof(unsigned long int));
            buflen+=sizeof(unsigned long int);
            break;
        case VAR_STR:
            temp=htonl(strlen((char *)getPointer()));
            memcpy(buf+buflen, &temp, sizeof(unsigned long int));
            buflen+=sizeof(unsigned long int);
            memcpy(buf+buflen, getPointer(), strlen((char *)getPointer()));
            buflen+=strlen((char *)getPointer());
    }

    return buflen;

}


// deserialize the content of a variable
// use the same order as above
int VarDef::deserialize(char *buf) {
    int bl=0;
    int t,tv, sl, ssl;
    char *s, *ss;

    //printf("VarDef: deserealization \n");

    memcpy(&t,buf+bl,sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    setType(ntohl(t));


    //printf("VarDef:: deserialization:: type done \n");
    
    memcpy(&tv,buf+bl,sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    sl = ntohl(tv);

    //printf("VarDef:: deserialization:: tv done \n");

    s=(char *)malloc(sl+1);
    memcpy(s,buf+bl,sl);
    bl+=sl;
    s[sl]='\0';
    setName(s);

    //printf("VarDef:: deserialization:: characters copying done \n");
    

    switch (getType()) {
        case VAR_INT:
            //printf("VarDef:: deserialization:: VAR_INT \n");
            memcpy(&tv, buf+bl,sizeof(unsigned long int));
            bl+=sizeof(unsigned long int);
            setValue(ntohl(tv));
            //printf("this is where it fails: \n");
            break;
        case VAR_STR:
            memcpy(&tv,buf+bl,sizeof(unsigned long int));
            bl+=sizeof(unsigned long int);
            ssl=ntohl(tv);
            ss=(char *)malloc(ssl+1);
            strncpy(ss,buf+bl,ssl);
            bl+=ssl;
            ss[ssl]='\0';
            setValue(ss);
            free(ss);
            break;
    }

    free(s);

    return bl;
}



// this is a function used for debug purposes. It prints the info
// about the variable
void VarDef::print() {
    //printf("Variable %s has type %d and value ",getName(), getType() );
    switch (getType()) {
        case VAR_INT:
            fprintf(stderr, "\n %d ; shadow %d", *((int *)getPointer()),*((int *)getShadow()), *((int*)getPointer()));
            break;
        case VAR_STR:
            fprintf(stderr, " [%s] ; shadow [%s], val [%s]",(char *)getPointer(), (char *)getShadow(), (char*)getPointer());
    }
    //fprintf(stderr, "\n");
    //fprintf(stderr, "Pointers: %p ; %p ; %p ; %p\n",getPointer(), getShadow(), getName(), &varType);
}



// serialize the general message class
int HMessage::serialize(char *buf) {
    int buflen=0;

    // include the message type and the version of the protocol
    memcpy(buf+buflen,&type, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    memcpy(buf+buflen,&version, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    //memcpy(buf+buflen,&flag, sizeof(unsigned long int));
    //buflen+=sizeof(unsigned long int);
    return buflen;
}


// deserialize the general message class
int HMessage::deserialize(char *buf) {
    int bl=0;

    // deserialize the message type and the version of the protocol
    memcpy(&type,buf+bl, sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    memcpy(&version,buf+bl, sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    //memcpy(&flag,buf+bl, sizeof(unsigned long int));
    //bl+=sizeof(unsigned long int);
    return bl;
}


// function used for debugging purposes that prints the type of the
// message
void HMessage::print() {
    fprintf(stderr, "Message type: %s %d\n",print_type[get_type()], get_type());
    //printf("Message type: %s %d\n",print_type[get_type()], get_type());
}



// serialize the Registration Message
int HRegistMessage::serialize(char *buf) {
    int buflen;

    buflen=HMessage::serialize(buf);

    // to the serialization of the general message add the param
    // that HRegistMessage keeps track of
    memcpy(buf+buflen,&param, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    memcpy(buf+buflen,&identification, sizeof(int));
    buflen+=sizeof(int);

    return buflen;
}


// deserialize the Registration Message
int HRegistMessage::deserialize(char *buf) {
    int bl=0;

    bl = HMessage::deserialize(buf);

    memcpy(&param,buf+bl, sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    memcpy(&identification, buf+bl, sizeof(int));
    bl+=sizeof(int);

    return bl;
}


// print extra information about the HRegistMessage
void HRegistMessage::print(){
    HMessage::print();
    //fprintf(stderr, "Parameter: %d\n",get_param());
    //printf("Parameter: %d\n",get_param());
}


// serialize the description message
int HDescrMessage::serialize(char *buf) {
    int buflen;

    buflen = HMessage::serialize(buf);

    // serialize the description (which is a string)
    // by first adding the length to the buffer and then
    // the actual string
    memcpy(buf+buflen,&descrlen, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);
    memcpy(buf+buflen,descr, get_descrlen());
    buflen+=get_descrlen();

    return buflen;
}


// deserialize the description message
int HDescrMessage::deserialize(char *buf) {
    int bl=0;

    bl = HMessage::deserialize(buf);

    memcpy(&descrlen,buf+bl, sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);
    free(descr);
    descr=(char *)malloc(get_descrlen()+1);
    memcpy(descr,buf+bl, get_descrlen());
    descr[get_descrlen()]='\0';
    bl+=get_descrlen();

    return bl;
}


// print info about the description message
void HDescrMessage::print() {
    //HMessage::print();
    fprintf(stderr, "Description has size %d: [%s]\n",get_descrlen(),get_descr());
}



// returns the variable n from the HUpdateMessage
VarDef * HUpdateMessage::get_var(int n) {

    vector<VarDef>::iterator VarIterator = var_list.begin();

    for (int i=0;i<n;i++,VarIterator++);

    return &*VarIterator;
}


// adds a variable to the HUpdateMessage
void HUpdateMessage::set_var(VarDef v) {
    //printf("HUpdateMessage::set_var \n");
    //printf("Set_Var!!!\n"); 

    //printf("var: %s\n",v.getName());

    set_nr_vars(get_nr_vars()+1);

    var_list.push_back(v);

    //printf("var pushed\n");
}


// compute the message size of the HUpdateMessage
// this depends on the length of each of the variables
// stored in the list of variables
// and that is why it could not be inlined.
int HUpdateMessage::get_message_size() {
    int len;

    len = HMessage::get_message_size()+2*sizeof(unsigned long int);

    vector<VarDef>::iterator VarIterator;

    for (VarIterator=var_list.begin(); VarIterator!=var_list.end(); VarIterator++) {

        VarDef var=*VarIterator;
        len+=sizeof(unsigned long int); // var type

        len+=sizeof(unsigned long int); // name len
        len+=strlen((char *)var.getName());

        switch (var.getType()) {
            case VAR_INT:
                len+=sizeof(unsigned long int); //value
                break;
            case VAR_STR:
                len+=sizeof(unsigned long int); //strlen
                len+=strlen((char *)var.getPointer()); // size of the string
                break;
        }
    }

    return len;
}


// serialize the HUpdateMessage
int HUpdateMessage::serialize(char *buf) {
    int buflen=0;
    unsigned long int temp;

    buflen=HMessage::serialize(buf);

    // add the timestamp to the buffer
    memcpy(buf+buflen, &timestamp, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);

    // add the number of the variables to the buffer
    memcpy(buf+buflen, &nr_vars, sizeof(unsigned long int));
    buflen+=sizeof(unsigned long int);

    vector<VarDef>::iterator VarIterator;

    // serialize the variables
    //printf("serializing the vardefs \n");
    for (VarIterator=var_list.begin(); VarIterator!=var_list.end(); VarIterator++)
        buflen+=(*VarIterator).serialize(buf+buflen);

    return buflen;
}


// deserialize the HUpdateMessage
int HUpdateMessage::deserialize(char *buf) {
    int bl=0;
    int t,tv, sl, ssl;
    char *s, *ss;
    VarDef *vv;
    unsigned long int tstamp;

    // printf("HUpdateMessage::deserialize(char *buf):: buf: %s\n", &buf);
    bl = HMessage::deserialize(buf);

    // printf("deserialization :: %s\n", &buf);
    // deserialize the timestamp
    memcpy(&tstamp,buf+bl, sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);

    set_timestamp(tstamp);

    // deserialize the number of variables
    memcpy(&t,buf+bl, sizeof(unsigned long int));
    bl+=sizeof(unsigned long int);

    tv=ntohl(t);

    // deserialize the variables
    for (int i=0; i<tv;i++) {
        vv = new VarDef();
        //printf("deserialize var: %d \n", i);
        bl+=vv->deserialize(buf+bl);
        //printf("deserialization done \n");
        //vv->print();
        //printf("deserialization done \n\n");
        set_var(*vv);
        //printf("deserialization done \n");
        //    if (debug_mode)
        //      vv->print();
        //vv->print();
    }
    //printf("done deserializing variables \n");
    return bl;
}


// print the info contained in the Update Message
// this means the variables sent for updating
void HUpdateMessage::print() {

    VarDef var;

    //fprintf(stderr, "PRINT UPDATE MESSAGE:\n");
    HMessage::print();
    // fprintf(stderr, "Nr vars: %d\n",get_nr_vars());
    //fprintf(stderr, "Timestamp: %d\n",get_timestamp());

    vector<VarDef>::iterator VarIterator;

    //  for(VarIterator=var_list.begin();VarIterator!=var_list.end();VarIterator++) {
    //    var=VarIterator;
    for (int i=0;i<get_nr_vars();i++)
    {
        (get_var(i))->print();
    }
    //fprintf(stderr, "DONE PRINTING!\n");
}
