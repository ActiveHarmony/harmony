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

#include "session-core.h"
#include "hmesg.h"
#include "hpoint.h"
#include "hutil.h"
#include "hcfg.h"
#include "hsockutil.h"
#include "defaults.h"

#define MY_ENCODING "ISO-8859-1"

char harmony_plugin_name[] = "xmlWriter";

//static clock_t file_create_time;
static char filename[128];
static char create_time[128];
static int paramNum;
//static int nodeNum;
static hsession_t session;
//static int first_pass = 1;
//void main();


void harmony_xmlWriteAppName(const char *appName);
void harmony_xmlWriteParamInfo(hmesg_t *mesg);
void harmony_xmlWritePerfData(hmesg_t *mesg);

int xmlWriter_init (hmesg_t *mesg) {
    int rc;
    xmlTextWriterPtr writer;
    //xmlChar *tmp;
    xmlDocPtr doc;
    xmlNodePtr node;
    
	/*Using create time to name xml file and initialize*/
	printf("Creating xml file.\n");
    time_t now;
    struct tm *current;
    now = time(0);
    current = localtime(&now);
    snprintf(create_time, 64, "%d%d%d", (int)current->tm_hour, (int)current->tm_min, (int)current->tm_sec);
	snprintf(filename, 128, "../log/%s_%s.xml", mesg->data.session.sig.name, create_time);
    doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);
    if (doc == NULL) {
		fprintf(stderr, "Error creating the xml document tree!\n");
		return -1;
    }
    xmlSaveFileEnc(filename, doc, MY_ENCODING);

	printf("XML file created: %s\n", filename);

    node = xmlNewDocNode(doc, NULL, BAD_CAST "Harmony", NULL);
    if (node == NULL) {
		fprintf(stderr,"Error creating the xml node\n");
		return -1;
    }

    //Make "HarmonyData" the root node of the tree
    xmlDocSetRootElement(doc, node);

    //Create a new xmlWriter for DOM tree, with no compression
    writer = xmlNewTextWriterTree(doc, node, 0);
    if (writer == NULL) {
		fprintf(stderr, "Error creating the xml writer.\n");
		return -1;
    }

    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
		fprintf(stderr, "Error at xmlTestWriterStartDocument\n");
		return -1;
    }
    rc = xmlTextWriterStartElement(writer, BAD_CAST "HarmonyData");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "Metadata");
    if (rc < 0) {
		fprintf(stderr, "Error at xmlTextWriterStartElement\n");
		return -1;
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

	/*Writing primary metadata*/
	hsession_copy(&session, &mesg->data.session);
	/*Write appName*/
	harmony_xmlWriteAppName(mesg->data.session.sig.name); 

	/*Write nodeInfo*/

	/*Write param info*/
	paramNum = mesg->data.session.sig.range_cap;
	harmony_xmlWriteParamInfo(mesg);

	return 0;
}

int xmlWriter_fetch(hmesg_t *mesg) {
	return 0;
}

int xmlWriter_report(hmesg_t *mesg) {
	harmony_xmlWritePerfData(mesg);
	return 0;
}

void harmony_xmlWriteNodeInfo(int clientID, char *nodeinfo, char *sysName, char *release, char *machine, int core_num, char *cpu_vendor, char *cpu_model, char *cpu_freq, char *cache_size) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    char proc_num[2];
    char client[4];

    snprintf(filename, 128, "%s.xml", create_time);
    snprintf(proc_num, 2, "%d", core_num);
    snprintf(client, 2, "%d", clientID);

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
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"ProcessorNum", (xmlChar *)proc_num);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"CPUVendor", (xmlChar *)cpu_vendor);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"CPUModel", (xmlChar *)cpu_model);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"CPUFreq", (xmlChar *)cpu_freq);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"CacheSize", (xmlChar *)cache_size);
	    xmlNewTextChild(curNode, NULL, (xmlChar *)"ClientID", (xmlChar *)client);

	    xmlSaveFileEnc(filename, doc, MY_ENCODING);
	    break;
	}
	curNode = curNode->next;
    }
}

    

void harmony_xmlWriteAppName(const char *appName) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

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
void harmony_xmlWriteParamInfo(hmesg_t *mesg) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    char paramName[32];
    char paramMin[16];
    char paramMax[16];
    char paramStep[16];


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
	   
		for (int i = 0; i < paramNum; i++) {
			snprintf(paramName, sizeof(paramName), "%s", session.sig.range[i].name);

			/*Type sensitive*/
			switch (session.sig.range[i].type) {
				case HVAL_INT:
					snprintf(paramMin, sizeof(paramMin), "%ld", session.sig.range[i].bounds.i.min);
					snprintf(paramMax, sizeof(paramMax), "%ld", session.sig.range[i].bounds.i.max);
					snprintf(paramStep, sizeof(paramStep), "%ld", session.sig.range[i].bounds.i.step);
					break;

				case HVAL_REAL:
					snprintf(paramMin, sizeof(paramMin), "%f", session.sig.range[i].bounds.r.min);
					snprintf(paramMax, sizeof(paramMax), "%f", session.sig.range[i].bounds.r.max);
					snprintf(paramStep, sizeof(paramStep), "%f", session.sig.range[i].bounds.r.step);
					break;

				case HVAL_STR:
				case HVAL_UNKNOWN:
				case HVAL_MAX:
					break;
			}

		    xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST"Param");
		    xmlAddChild(curNode, newNode);
		    
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramName", (xmlChar *)paramName);
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramMin", (xmlChar *)paramMin);
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramMax", (xmlChar *)paramMax);
		    xmlNewTextChild(newNode, NULL, (xmlChar *)"paramStep", (xmlChar *)paramStep);
		    xmlSaveFileEnc(filename, doc, MY_ENCODING);
		}
	    
	}
	curNode = curNode->next;
    }
}


void harmony_xmlWritePerfData(hmesg_t *mesg) {
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *dataNode;
    xmlNode *root_element = NULL;
    char client[4];
    snprintf(client, 4, "%d", mesg->dest);

    char cur_time[64];
    char temp[32];
    char curConfig[32];
    time_t now;
    struct tm *current;

	long min_i, step_i, val_i;
	double min_r, step_r, val_r;
	int index;

    now = time(0);
    current = localtime(&now);
    snprintf(cur_time, 64, "%d:%d:%d", current->tm_hour, current->tm_min, current->tm_sec);


    char performance[32];
    sprintf(performance, "%lf", mesg->data.report.perf);

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
	    
	    for (int i = 0; i < paramNum; i++) {
			snprintf(temp, sizeof(temp), "%s", session.sig.range[i].name);
			/*Type sensitive*/
			index = mesg->data.report.cand.idx[i];
			switch (session.sig.range[i].type) {
				case HVAL_INT:
					min_i = session.sig.range[i].bounds.i.min;
					step_i = session.sig.range[i].bounds.i.step;
					val_i = min_i + step_i * index;
					snprintf(curConfig, sizeof(curConfig), "%ld", val_i);
					break;

				case HVAL_REAL:
					min_r = session.sig.range[i].bounds.r.min;
					step_r = session.sig.range[i].bounds.r.step;
					val_r = min_r + step_r * index;
					snprintf(curConfig, sizeof(curConfig), "%f", val_r);
					break;			
				case HVAL_STR:
					snprintf(curConfig, sizeof(curConfig), "%s", session.sig.range[i].bounds.e.set[index]);
					break;

				case HVAL_UNKNOWN:
				case HVAL_MAX:
					break;
			}
		    xmlNewTextChild(curNode, NULL, (xmlChar *)temp, (xmlChar *)curConfig);

	    }

	    xmlNewTextChild(dataNode, NULL, (xmlChar *)"Perf", (xmlChar *)performance);
	    xmlNewTextChild(dataNode, NULL, (xmlChar *)"Time", (xmlChar *)cur_time);
	    xmlNewTextChild(dataNode, NULL, (xmlChar *)"Client", (xmlChar *)client);
	    xmlSaveFileEnc(filename, doc, MY_ENCODING);
	    break;
	}
	curNode = curNode->next;
    }
}
