/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
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

/**
 * \page xml XML Writer (xmlWriter.so)
 *
 * \warning This processing layer is still considered pre-beta.
 *
 * This processing layer writes an XML formatted log of
 * point/performance pairs to disk as they flow through the
 * auto-tuning [feedback loop](\ref intro_feedback).
 *
 * The `LIBXML2` [build variable](\ref start_install) must be defined
 * at build time for this plug-in to be available.  The libxml2
 * library is available in multiple forms here:
 * - http://www.xmlsoft.org/downloads.html
 *
 * And `LIBXML2` should point wherever libxml2 has been installed.
 * For Linux distributions that include libxml2 as a package, using
 * `/usr` may be sufficient.
 *
 * **Configuration Variables**
 * Key                | Type    | Default | Description
 * ------------------ | ------- | ------- | -----------
 * XML_FILENAME       | String  | [none]  | XML output file.
 */

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
#include "hsignature.h"
#include "hpoint.h"
#include "hutil.h"
#include "defaults.h"

#define MY_ENCODING "ISO-8859-1"

char harmony_layer_name[] = "xmlWriter";

hsignature_t sess_sig;
/* clock_t file_create_time; */
char filename[128];
char create_time[128];
int paramNum;

int harmony_xmlWriteAppName(const char *appName);
int harmony_xmlWriteParamInfo(void);

int xmlWriter_init(hsignature_t *sig)
{
    int rc;
    const char *tmpstr;
    xmlTextWriterPtr writer;
    /* xmlChar *tmp; */
    xmlDocPtr doc;
    xmlNodePtr node;

    /* Using create time to name xml file and initialize */
    time_t now;
    struct tm *current;
    now = time(0);
    current = localtime(&now);
    snprintf(create_time, 64, "%d%d%d", (int)current->tm_hour,
             (int)current->tm_min, (int)current->tm_sec);

    tmpstr = session_getcfg("XML_FILENAME");
    if (tmpstr)
        strncpy(filename, tmpstr, sizeof(filename));
    else
        snprintf(filename, sizeof(filename), "%s_%s.xml",
                 sig->name, create_time);

    doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION);
    if (!doc) {
        session_error("Error creating the xml document tree");
        return -1;
    }
    xmlSaveFileEnc(filename, doc, MY_ENCODING);

    node = xmlNewDocNode(doc, NULL, BAD_CAST "Harmony", NULL);
    if (node == NULL) {
        fprintf(stderr,"Error creating the xml node\n");
        return -1;
    }

    /* Make "HarmonyData" the root node of the tree */
    xmlDocSetRootElement(doc, node);

    /* Create a new xmlWriter for DOM tree, with no compression */
    writer = xmlNewTextWriterTree(doc, node, 0);
    if (!writer) {
        session_error("Error creating the xml writer");
        return -1;
    }

    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        session_error("Error at xmlTestWriterStartDocument");
        return -1;
    }
    rc = xmlTextWriterStartElement(writer, BAD_CAST "HarmonyData");
    rc = xmlTextWriterStartElement(writer, BAD_CAST "Metadata");
    if (rc < 0) {
        session_error("Error at xmlTextWriterStartElement");
        return -1;
    }

    rc = xmlTextWriterStartElement(writer, BAD_CAST "Nodeinfo");

    /* Close the element Nodeinfo */
    rc = xmlTextWriterEndElement(writer);

    /* Start them element AppName */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "AppName");

    /* Close the element AppName */
    rc = xmlTextWriterEndElement(writer);

    /* Start and close the element Param */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "ParamList");
    rc = xmlTextWriterEndElement(writer);

    /* Start and clost starttime */
    rc = xmlTextWriterWriteFormatElement(writer, BAD_CAST "StartTime",
                                         "%s", create_time);
                                         /* Close the element Metadata */
    rc = xmlTextWriterEndElement(writer);

    /* Start and close RawData */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "RawData");
    rc = xmlTextWriterEndElement(writer);

    /* close HarmonyData */
    rc = xmlTextWriterEndElement(writer);

    rc = xmlTextWriterEndDocument(writer);

    xmlFreeTextWriter(writer);
    xmlSaveFileEnc(filename, doc, MY_ENCODING);
    xmlFreeDoc(doc);

    /* Writing primary metadata */
    hsignature_copy(&sess_sig, sig);

    /* Write appName */
    if (harmony_xmlWriteAppName(sig->name) != 0)
        return -1;

    /* Write nodeInfo */

    /* Write param info */
    paramNum = sig->range_len;
    if (harmony_xmlWriteParamInfo() != 0)
        return -1;

    return 0;
}

int xmlWriter_generate(hflow_t *flow, htrial_t *trial)
{
    int i;

    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *dataNode;
    xmlNode *root_element = NULL;

    char timestr[64];
    char temp[32];
    char confstr[32];
    time_t now;
    struct tm *current;

    now = time(0);
    current = localtime(&now);
    snprintf(timestr, 64, "%d:%d:%d", current->tm_hour,
             current->tm_min, current->tm_sec);

    char performance[32];
    sprintf(performance, "%lf", trial->perf);

    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
        session_error("Failed to parse the xml file");
        return -1;
    }

    root_element = xmlDocGetRootElement(doc);
    /* point to the first child under HarmonyData tag */
    curNode = root_element->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
        if (xmlStrcmp(curNode->name, (const xmlChar *)"RawData") == 0) {
            xmlNodePtr newNode;

            newNode = xmlNewNode(NULL, BAD_CAST"Data");
            xmlAddChild(curNode, newNode);
            curNode = newNode;

            dataNode = newNode;

            newNode = xmlNewNode(NULL, BAD_CAST"Config");
            xmlAddChild(curNode, newNode);
            curNode = newNode;

            /* Start adding param and corresponding config */
            for (i = 0; i < paramNum; i++) {
                hval_t *val = &trial->point.val[i];

                snprintf(temp, sizeof(temp), "%s", sess_sig.range[i].name);
                switch (val->type) {
                case HVAL_INT:
                    snprintf(confstr, sizeof(confstr), "%ld", val->value.i);
                    break;

                case HVAL_REAL:
                    snprintf(confstr, sizeof(confstr), "%f", val->value.r);
                    break;

                case HVAL_STR:
                    snprintf(confstr, sizeof(confstr), "%s", val->value.s);
                    break;

                default:
                    break;
                }
                xmlNewTextChild(curNode, NULL, (xmlChar *)temp,
                                (xmlChar *)confstr);
            }

            xmlNewTextChild(dataNode, NULL, (xmlChar *)"Perf",
                            (xmlChar *)performance);
            xmlNewTextChild(dataNode, NULL, (xmlChar *)"Time",
                            (xmlChar *)timestr);
            xmlNewTextChild(dataNode, NULL, (xmlChar *)"Client",
                            (xmlChar *)session_getcfg(CFGKEY_CURRENT_CLIENT));
            xmlSaveFileEnc(filename, doc, MY_ENCODING);
            break;
        }
        curNode = curNode->next;
    }
    return 0;
}

int harmony_xmlWriteNodeInfo(int clientID, char *nodeinfo, char *sysName,
                             char *release, char *machine, int core_num,
                             char *cpu_vendor, char *cpu_model,
                             char *cpu_freq, char *cache_size)
{
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    char proc_num[2];
    char client[4];

    snprintf(filename, 128, "%s.xml", create_time);
    snprintf(proc_num, 2, "%d", core_num);
    snprintf(client, 2, "%d", clientID);

    doc = xmlReadFile(filename, NULL, 0);
    if (!doc) {
        session_error("Failed to parse the xml file");
        return -1;
    }

    root_element = xmlDocGetRootElement(doc);

    curNode = root_element->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
        if (!xmlStrcmp(curNode->name, (const xmlChar *)"Nodeinfo")) {
            xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST"Node");
            xmlAddChild(curNode, newNode);
            curNode = newNode;
            xmlNewTextChild(curNode, NULL, (xmlChar *)"HostName",
                            (xmlChar *)nodeinfo);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"sysName",
                            (xmlChar *)sysName);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"Release",
                            (xmlChar *)release);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"Machine",
                            (xmlChar *)machine);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"ProcessorNum",
                            (xmlChar *)proc_num);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"CPUVendor",
                            (xmlChar *)cpu_vendor);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"CPUModel",
                            (xmlChar *)cpu_model);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"CPUFreq",
                            (xmlChar *)cpu_freq);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"CacheSize",
                            (xmlChar *)cache_size);
            xmlNewTextChild(curNode, NULL, (xmlChar *)"ClientID",
                            (xmlChar *)client);

            xmlSaveFileEnc(filename, doc, MY_ENCODING);
            break;
        }
        curNode = curNode->next;
    }
    return 0;
}

int harmony_xmlWriteAppName(const char *appName)
{
    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    doc = xmlReadFile(filename, NULL, 0);
    if (!doc) {
        session_error("Failed to parse the xml file");
        return -1;
    }

    root_element = xmlDocGetRootElement(doc);
    //point to the first child under Metadata tag
    curNode = root_element->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode;
    while (curNode) {
        if (!xmlStrcmp(curNode->name, (const xmlChar *)"AppName")) {
            xmlNewTextChild(curNode, NULL, (xmlChar *)"appName",
                            (xmlChar *)appName);
            xmlSaveFileEnc(filename, doc, MY_ENCODING);
            break;
        }
        curNode = curNode->next;
    }
    return 0;
}

/* This function get the param information in the followint format
 * {Name Min Max Step Name Min Max Step ......}  And then parse this
 * formation and put it into the Metadata section of the xml file
 */
int harmony_xmlWriteParamInfo(void)
{
    int i;

    xmlDoc *doc;
    xmlNode *curNode;
    xmlNode *root_element = NULL;

    char paramName[32];
    char paramMin[16];
    char paramMax[16];
    char paramStep[16];

    doc = xmlReadFile(filename, NULL, 0);
    if (doc == NULL) {
        session_error("Failed to parse the xml file");
        return -1;
    }

    root_element = xmlDocGetRootElement(doc);

    /* Point to the first child under Metadata tag */
    curNode = root_element->xmlChildrenNode->xmlChildrenNode->xmlChildrenNode;
    while (curNode != NULL) {
        if (!xmlStrcmp(curNode->name, (const xmlChar *)"ParamList")) {

            for (i = 0; i < paramNum; i++) {
                snprintf(paramName, sizeof(paramName), "%s",
                         sess_sig.range[i].name);

                /*Type sensitive*/
                switch (sess_sig.range[i].type) {
                case HVAL_INT:
                    snprintf(paramMin, sizeof(paramMin), "%ld",
                             sess_sig.range[i].bounds.i.min);
                    snprintf(paramMax, sizeof(paramMax), "%ld",
                             sess_sig.range[i].bounds.i.max);
                    snprintf(paramStep, sizeof(paramStep), "%ld",
                             sess_sig.range[i].bounds.i.step);
                    break;

                case HVAL_REAL:
                    snprintf(paramMin, sizeof(paramMin), "%f",
                             sess_sig.range[i].bounds.r.min);
                    snprintf(paramMax, sizeof(paramMax), "%f",
                             sess_sig.range[i].bounds.r.max);
                    snprintf(paramStep, sizeof(paramStep), "%f",
                             sess_sig.range[i].bounds.r.step);
                    break;

                case HVAL_STR:

                default:
                    break;
                }

                xmlNodePtr newNode = xmlNewNode(NULL, BAD_CAST"Param");
                xmlAddChild(curNode, newNode);

                xmlNewTextChild(newNode, NULL, (xmlChar *)"paramName",
                                (xmlChar *)paramName);
                xmlNewTextChild(newNode, NULL, (xmlChar *)"paramMin",
                                (xmlChar *)paramMin);
                xmlNewTextChild(newNode, NULL, (xmlChar *)"paramMax",
                                (xmlChar *)paramMax);
                xmlNewTextChild(newNode, NULL, (xmlChar *)"paramStep",
                                (xmlChar *)paramStep);
                xmlSaveFileEnc(filename, doc, MY_ENCODING);
            }

        }
        curNode = curNode->next;
    }
    return 0;
}
