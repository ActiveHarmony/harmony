#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlwriter.h>

#include "record.h"

#define MY_ENCODING "ISO-8859-1"


//static clock_t file_create_time;
char filename[128];
const char create_time[128];
static int first_pass = 1;
//void main();

void init_ref_file (int clientID) {
    int rc;
    xmlTextWriterPtr writer;
    //xmlChar *tmp;
    xmlDocPtr doc;
    xmlNodePtr node;
    //char create_time[64];
    time_t now;
    struct tm *current;
    now = time(0);
    current = localtime(&now);
    snprintf(create_time, 64, "%d%d%d", (int)current->tm_hour, (int)current->tm_min, (int)current->tm_sec);

    snprintf(filename, 128, "%s_%d.xml", create_time, clientID);
    doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);
    if (doc == NULL) {
	printf("Error creating the xml document tree!\n");
	exit(1);
    }

    xmlSaveFileEnc(filename, doc, MY_ENCODING);

    node = xmlNewDocNode(doc, NULL, BAD_CAST "Harmony", NULL);
    if (node == NULL) {
	printf("Error creating the xml node\n");
	exit(1);
    }

    //Make "HarmonyData" the root node of the tree
    xmlDocSetRootElement(doc, node);

    //Create a new xmlWriter for DOM tree, with no compression
    writer = xmlNewTextWriterTree(doc, node, 0);
    if (writer == NULL) {
	printf("Error creating the xml writer.\n");
	exit(1);
    }

    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
	printf("Error at xmlTestWriterStartDocument\n");
	exit(1);
    }
    rc = xmlTextWriterStartElement(writer, BAD_CAST "HarmonyData");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "Metadata");
    if (rc < 0) {
	printf("Error at xmlTextWriterStartElement\n");
	exit(1);
    }

    rc = xmlTextWriterStartElement(writer, BAD_CAST "Nodeinfo");

    //Close the element Nodeinfo
    rc = xmlTextWriterEndElement(writer);

    //Start them element AppName
    rc = xmlTextWriterStartElement(writer, BAD_CAST "AppName");

    //Close the element AppName
    rc = xmlTextWriterEndElement(writer);

    //Start and close the element Param
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ParamList");
    rc = xmlTextWriterEndElement(writer);

    //Start and clost starttime
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "StartTime", "%s", create_time);
    //Close the element Metadata
    rc = xmlTextWriterEndElement(writer);

    //Start and close RawData
    rc = xmlTextWriterStartElement(writer, BAD_CAST "RawData");
    rc = xmlTextWriterEndElement(writer);
    
    //clost HarmonyData
    rc = xmlTextWriterEndElement(writer);

    rc = xmlTextWriterEndDocument(writer);

    xmlFreeTextWriter(writer);
    xmlSaveFileEnc(filename, doc, MY_ENCODING);
    xmlFreeDoc(doc);
}

void write_nodeinfo(int clientID, char *nodeinfo, char *sysName, char *release, char *machine) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    snprintf(filename, 128, "%s_%d.xml", create_time, clientID);

    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
	fprintf(stderr, "Failed to parse the xml file!\n");
	return;
    }

    root_element = xmlDocGetRootElement(doc);

    curNode = root_element->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
	if (!xmlStrcmp(curNode->name, (const xmlChar *)"Nodeinfo")) {
	    xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST"Node");
	    xmlAddChild(curNode, newNode);
	    curNode = newNode;
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"HostName", (xmlChar *)nodeinfo);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"sysName", (xmlChar *)sysName);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"Release", (xmlChar *)release);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"Machine", (xmlChar *)machine);
	    xmlSaveFileEnc(filename, doc, MY_ENCODING);
	    break;
	}
	curNode = curNode->next;
    }
}

    

void write_appName(int clientID, const char *appName) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    snprintf(filename, 128, "%s_%d.xml", create_time, clientID);

    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
	fprintf(stderr, "Failed to parse the xml file!\n");
	return;
    }

    root_element = xmlDocGetRootElement(doc);
    //point to the first child under Metadata tag
    curNode = root_element->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
	if (!xmlStrcmp(curNode->name, (const xmlChar *)"AppName")) {
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"appName", (xmlChar *)appName);
	    xmlSaveFileEnc(filename, doc, MY_ENCODING);
	    break;
	}
	curNode = curNode->next;
    }
}

/*This function get the param information in the followint format {Name Min Max Step Name Min Max Step ......}
And then parse this formation and put it into the Metadata section of the xml file*/
void write_param_info(int clientID, char *paramInfo) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    snprintf(filename, 128, "%s_%d.xml", create_time, clientID);

    char paramName[32];
    char paramMin[16];
    char paramMax[16];
    char paramStep[16];

    int i = 0, j = 0;
    bool EndLine;
    EndLine = false;


    if (strlen(paramInfo) == 0) {
	fprintf(stderr, "No Param Info found!\n");
	return;
    }

    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
	fprintf(stderr, "Failed to parse the xml file!\n");
	return;
    }

    root_element = xmlDocGetRootElement(doc);
    
    //Point to the first child under Metadata tag
    curNode = root_element->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
	if (!xmlStrcmp(curNode->name, (const xmlChar *)"ParamList")) {
	   
	    if (first_pass == 1) {
		first_pass = 0;
		while (i < strlen(paramInfo)) {
		    while ((!isspace(paramInfo[j])) && (j <= strlen(paramInfo)-1)) {
			j++;
		    }
		    strncpy(paramName, &paramInfo[i], j-i);
		    paramName[j-i] = '\0';
		    i = ++j;

		    while ((!isspace(paramInfo[j])) && (j <= strlen(paramInfo)-1)) {
			j++;
		    }
		    strncpy(paramMin, &paramInfo[i], j-i);
		    paramMin[j-i] = '\0';
		    i = ++j;

		    while ((!isspace(paramInfo[j])) && (j <= strlen(paramInfo)-1)) {
			j++;
		    }
		    strncpy(paramMax, &paramInfo[i], j-i);
		    paramMax[j-i] = '\0';
		    i = ++j;

		    while ((!isspace(paramInfo[j])) && (j <= strlen(paramInfo)-1)) {
			j++;
		    }
		    strncpy(paramStep, &paramInfo[i], j-i);
		    paramStep[j-i] = '\0';
		    i = ++j;
		    //break;

		    xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST"Param");
		    xmlAddChild(curNode, newNode);
		    
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramName", (xmlChar *)paramName);
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramMin", (xmlChar *)paramMin);
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramMax", (xmlChar *)paramMax);
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramStep", (xmlChar *)paramStep);
		    xmlSaveFileEnc(filename, doc, MY_ENCODING);
		}
	    }
	    
	}
	curNode = curNode->next;
    }
}


void write_conf_perf_pair(int clientID, const char *param_namelist, const char *config, double perf) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *dataNode;
    xmlNode *root_element = NULL;

    snprintf(filename, 128, "%s_%d.xml", create_time, clientID);

    bool EndLine;
    char cur_time[64];
    char temp[32];
    char curConfig[32];
    time_t now;
    struct tm *current;
    int i = 0, j = 0, m = 0, n = 0;

    EndLine = false;

    now = time(0);
    current = localtime(&now);
    snprintf(cur_time, 64, "%d:%d:%d", current->tm_hour, current->tm_min, current->tm_sec);


    char performance[32];
    sprintf(performance, "%lf", perf);

    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
	fprintf(stderr, "Failed to parse the xml file!\n");
	return;
    }

    root_element = xmlDocGetRootElement(doc);
    //point to the first child under HarmonyData tag
    curNode = root_element->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
	if (!xmlStrcmp(curNode->name, (const xmlChar *)"RawData")) {
	    xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST"Data");
	    xmlAddChild(curNode, newNode);
	    curNode = newNode;
	    dataNode = newNode;
	    newNode = xmlNewNode(NULL, BAD_CAST"Config");
	    xmlAddChild(curNode, newNode);
	    curNode = newNode;

	    //Start adding param and corresponding config
	    
	    while(EndLine == false) {
  	        while (i < strlen(param_namelist)-1) { 
		    while ((!isspace(param_namelist[j])) && (j <= strlen(param_namelist)-1)) {
		        j++;
		    }
		    strncpy(temp, &param_namelist[i], j-i);
		    temp[j-i] = '\0';
		    printf("config loaded: %s\n",temp);
		    i = j;
		    break;
	        } 
	    
	        while (m <= strlen(config)-1) {
		    while (!isspace(config[n]) && (n <= strlen(config)-1)) {
		        n++;
		    }
		    strncpy(curConfig, &config[m], n-m);
		    curConfig[n-m] = '\0';
		    printf("config value loaded: %s\n",curConfig);
		    m = n;

		    xmlNewTextChild(curNode, NULL, (xmlChar *)temp, (xmlChar *)curConfig);
		    break;
	        }

		if(i >= strlen(param_namelist) && m >= strlen(config)) {
		    EndLine = true;
		} else {
		    i++;
		    j++;
		    m++;
		    n++;
		}
	    }

	    xmlNewTextChild(dataNode, NULL, (xmlChar *)"Perf", (xmlChar *)performance);
	    xmlNewTextChild(dataNode, NULL, (xmlChar *)"Time", (xmlChar *)cur_time);
	    xmlSaveFileEnc(filename, doc, MY_ENCODING);
	    break;
	}
	curNode = curNode->next;
    }
}
