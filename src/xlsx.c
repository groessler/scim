#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>   // for isdigit
#include "macros.h"
#include "sc.h"
#include "cmds.h"
#include "color.h"
#include "lex.h"

#ifdef XLSX
#include <zip.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "xlsx.h"

// ------------------------------------------------------------------
// requires libzip-dev
// requires libxml2-dev
// for building: gcc -lzip -lxml2 -I/usr/include/libxml2 xlsx.c
// ------------------------------------------------------------------


// this functions takes the DOM of the sharedStrings file
// and based on a position, it returns the according string
// note that 0 is the first string.
char * get_xlsx_string(xmlDocPtr doc, int pos) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;

    while (pos--) cur_node = cur_node->next;
    cur_node = cur_node->xmlChildrenNode; return (char *) cur_node->xmlChildrenNode->content;
}

// this functions takes the DOM of the styles file
// and based on a position, it returns the according numFmtId
// note that 0 is the first "xf".
char * get_xlsx_styles(xmlDocPtr doc_styles, int pos) {
    // we go forward up to styles data
    xmlNode * cur_node = xmlDocGetRootElement(doc_styles)->xmlChildrenNode;
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "cellXfs")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;
    // we go forward up to desidered numFmtId
    while (pos--) cur_node = cur_node->next;
    char * id = (char *) xmlGetProp(cur_node, (xmlChar *) "numFmtId");
    return id;
}

// this function takes the sheetfile DOM and builds the tbl spreadsheet (SCIM format)
void get_sheet_data(xmlDocPtr doc, xmlDocPtr doc_strings, xmlDocPtr doc_styles) {
    xmlNode * cur_node = xmlDocGetRootElement(doc)->xmlChildrenNode;
    xmlNode * child_node = NULL;
    char line_interp[FBUFLEN] = "";    
    int r, c;

    // we go forward up to sheet data
    while (cur_node != NULL && !(cur_node->type == XML_ELEMENT_NODE && !strcmp((char *) cur_node->name, "sheetData")))
        cur_node = cur_node->next;

    cur_node = cur_node->xmlChildrenNode;       // this is sheetdata
    while (cur_node != NULL) {
        child_node = cur_node->xmlChildrenNode; // this are rows
        while (child_node != NULL) {            // this are cols
            char * row = (char *) xmlGetProp(cur_node, (xmlChar *) "r");
            r = atoi(row) - 1;
            //printf("++: %s ", child_node->name);
            char * col = (char *) xmlGetProp(child_node, (xmlChar *) "r");
            while (isdigit(col[strlen(col)-1])) col[strlen(col)-1]='\0';
            c = atocol(col, strlen(col));

            if (xmlHasProp(child_node, (xmlChar *) "t")) {
                char * s = (char *) xmlGetProp(child_node, (xmlChar *) "t");
                char * style = (char *) xmlGetProp(child_node, (xmlChar *) "s");
                char * fmtId = get_xlsx_styles(doc_styles, atoi(style));

                // number
                if ( ! strcmp(s, "n") ) {
                    // v - straight int value
                    if (! strcmp((char *) child_node->xmlChildrenNode->name, "v") && strcmp(fmtId, "165")) {
                        //double l = strtol((char *) child_node->xmlChildrenNode->xmlChildrenNode->content, (char **) NULL, 10);
                        double l = atof((char *) child_node->xmlChildrenNode->xmlChildrenNode->content);
                        sprintf(line_interp, "let %s%d=%.15f", coltoa(c), r, l);
                        send_to_interp(line_interp);

                    // date value in v
                    } else if (! strcmp((char *) child_node->xmlChildrenNode->name, "v") && !strcmp(fmtId, "165")) {
                        long l = strtol((char *) child_node->xmlChildrenNode->xmlChildrenNode->content, (char **) NULL, 10);
                        sprintf(line_interp, "let %s%d=%.15ld", coltoa(c), r, (l - 25568) * 86400);
                        send_to_interp(line_interp);
                        struct ent * n = lookat(r, c);
                        n->format = 0;
                        char * stringFormat = scxmalloc((unsigned)(strlen("%d/%m/%Y") + 2));
                        sprintf(stringFormat, "%c", 'd');
                        strcat(stringFormat, "%d/%m/%Y");
                        n->format = stringFormat;

                    // f - numeric value is result from formula
                    } else if (! strcmp((char *) child_node->xmlChildrenNode->name, "f")) {
                        //double l = strtol((char *) child_node->last->xmlChildrenNode->content, (char **) NULL, 10);
                        double l = atof((char *) child_node->last->xmlChildrenNode->content);
                        sprintf(line_interp, "let %s%d=%.15f", coltoa(c), r, l);
                        send_to_interp(line_interp);
                    } 

                // string
                } else if ( ! strcmp(s, "s") ) {
                    sprintf(line_interp, "label %s%d=\"%s\"", coltoa(c), r, 
                    get_xlsx_string(doc_strings, atoi((char *) child_node->xmlChildrenNode->xmlChildrenNode->content)));
                    send_to_interp(line_interp);
                }

                xmlFree(s);
                xmlFree(fmtId);
                xmlFree(style);
            }
            child_node = child_node->next;
            xmlFree(col);
            xmlFree(row);
        }
        cur_node = cur_node->next;
    }
    return;
}

int open_xlsx(char * fname, char * encoding) {
    struct zip * za;
    struct zip_file * zf;
    struct zip_stat sb, sb_strings, sb_styles;
    char buf[100];
    int err;
    int len;
    
    // open zip file
    if ((za = zip_open(fname, 0, &err)) == NULL) {
        zip_error_to_str(buf, sizeof(buf), err, errno);
        error("can't open zip archive `%s': %s", fname, buf);
        return -1;
    }
 
    // open xl/sharedStrings.xml
    char * name = "xl/sharedStrings.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if (! zf) {
        error("cannot open %s file.\n", name);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_strings);
    char * strings = (char *) malloc(sb_strings.size);
    len = zip_fread(zf, strings, sb_strings.size);
    if (len < 0) {
        error("cannot read file %s.\n", name);
        free(strings);
        return -1;
    }
    zip_fclose(zf);

    // open xl/styles.xml
    name = "xl/styles.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        error("cannot open %s file.", name);
        free(strings);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb_styles);
    char * styles = (char *) malloc(sb_styles.size);
    len = zip_fread(zf, styles, sb_styles.size);
    if (len < 0) {
        error("cannot read file %s.", name);
        free(strings);
        free(styles);
        return -1;
    }
    zip_fclose(zf);


    // open xl/worksheets/sheet1.xml
    name = "xl/worksheets/sheet1.xml";
    zf = zip_fopen(za, name, ZIP_FL_UNCHANGED);
    if ( ! zf ) {
        error("cannot open %s file.", name);
        free(strings);
        free(styles);
        return -1;
    }
    zip_stat(za, name, ZIP_FL_UNCHANGED, &sb);
    char * sheet = (char *) malloc(sb.size);
    len = zip_fread(zf, sheet, sb.size);
    if (len < 0) {
        error("cannot read file %s.", name);
        free(strings);
        free(styles);
        free(sheet);
        return -1;
    }
    zip_fclose(zf);


    // XML parse for the sheet file
    xmlDoc * doc = NULL;
    xmlDoc * doc_strings = NULL;
    xmlDoc * doc_styles = NULL;

    // this initialize the library and check potential ABI mismatches
    // between the version it was compiled for and the actual shared
    // library used.
    LIBXML_TEST_VERSION

    // parse the file and get the DOM
    doc_strings = xmlReadMemory(strings, sb_strings.size, "noname.xml", NULL, 0);
    doc_styles = xmlReadMemory(styles, sb_styles.size, "noname.xml", NULL, 0);
    doc = xmlReadMemory(sheet, sb.size, "noname.xml", NULL, 0);

    if (doc == NULL || doc_strings == NULL) {
        error("error: could not parse file");
        free(strings);
        free(styles);
        free(sheet);
        return -1;
    }

    get_sheet_data(doc, doc_strings, doc_styles);

    // free the document
    xmlFreeDoc(doc);
    xmlFreeDoc(doc_strings);
    xmlFreeDoc(doc_styles);

    // Free the global variables that may have been allocated by the parser
    xmlCleanupParser();

    // free both sheet and strings variables
    free(strings);
    free(styles);
    free(sheet);

    // close zip file
    if (zip_close(za) == -1) {
        error("cannot close zip archive `%s'", fname);
        return -1;
    }
 
    auto_justify(0, maxcol, DEFWIDTH);
    return 0;
}
#endif
