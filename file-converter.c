#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlschemastypes.h>
/*
	gcc dom-converter.c -I/usr/include/libxml2 -I/usr/include/json-c  -std=c99 -o dom-converter -ljson-c -lxml2
	./dom-converter <input file> <out file> <operation>
	exp; ./dom-converter webapp.json webapp.xml 4
*/
#define FIELDSIZE 20
#define BUFFSIZE 200

typedef struct Node{			/*Node Structure like linkedlist for json and xml conversations*/
    json_object *json_object;
    const xmlChar *name;
	xmlNode *node;
    struct Node *next;
    struct Node *previous;
}Node;

typedef struct List{  			/*Created List, List struct keep List head, tail and index*/
    Node *head;
	 Node *tail;
	int nodeIndex;
}List;

typedef struct List{  			/*Created List, List struct keep List head, tail and index*/
    Node *head;
	 Node *tail;
	char nodeIndex;
}csvList;

struct {						/*json_flags Struct, keep flag id for exclude to file operions and operations assign below.*/
	  int flag;
	  const char *flag_str;
   } json_flags[] = {
		{ JSON_C_TO_STRING_PLAIN,  "JSON_C_TO_STRING_PLAIN" },	/*JSON_C_TO_STRING_PLAIN, W+ plain string */
		{ JSON_C_TO_STRING_SPACED, "JSON_C_TO_STRING_SPACED" }, /*JSON_C_TO_STRING_SPACED, W+ spaced string, there is no trim */
		{ JSON_C_TO_STRING_PRETTY, "JSON_C_TO_STRING_PRETTY" },	/*JSON_C_TO_STRING_PRETTY, W+ ideal json format*/
		{ JSON_C_TO_STRING_NOZERO, "JSON_C_TO_STRING_NOZERO" }, /*JSON_C_TO_STRING_NOZERO, W+ no exist null string*/
		{ JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY, "JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY" }, /* Whe use this flag, We define JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY */
		{ -1, NULL }
	};


char* csvFile;         			 /*File Buffer Size*/
char** csvColumnName; 			 /*CVS File Field Contents*/
int columnCount = 0;
int csvColumnNumber = 0;       /*Keeps column size of a csv file.*/
int objectCounter = 0;		    /*For JSON opereations, keep json object count*/
int elementCounter = 0;		    /*Keep array and object arrays element count*/
int jsonArrayCounter = 0;	    /*For JSON opereations, keep json array count*/

int findElementSize(xmlNode *node);
int isThereSibling(List *list, const xmlChar *name, xmlNode *node); /*isThereSibling, e*/
int isArray(xmlNode *node);
int csvGetLine(FILE *fin);
char *trimWhitespace(char * string);
void createList(List *list);
void addList(List *list, json_object *jobj,const xmlChar *name, xmlNode *node);
void JsonParser(json_object *jsonObject, xmlNode *xmlnode);
void JsonParserArray(json_object *jsonObject, char *key, xmlNode *xmlnode);
void csvToXML(const char *filename, const char *out_file);
void csvToJSON(const char *filename, const char *out_name);
void xmlToJson(xmlNode *node, List *obj_list, json_object *json_obj[], List *object_arr, json_object *json_arr[]);
static char** csvGetRow();
static csvRow* csvParseFile();
static void xmlToCsv(xmlNode *xml_node);
json_object *getJsonObject(List *list, const xmlChar *name);
xmlNode *findNode(xmlNode *xmlnode);

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 4) printf("Argument Error: Please Enter Right Argument");
   if(strstr(argv[3],  "1") != NULL){  // csv to xml operation
		 csvToXML(argv[1],argv[2]);
	}
	if(strstr(argv[3], "2") != NULL){
		xmlDocPtr doc = NULL;
		xmlNode *root_element = NULL;
		xmlNode *next_element = NULL;
		csvFile = (char*)malloc(sizeof(char) * 50000);
		FILE *fp = fopen(outputFile, "w");
		doc = xmlReadFile(argv[1]);
		if (doc == NULL){
			printf("Error: %s has not been found. Please check the file and try again.\n", argv[1]);
			exit(0);
		}
		root_element = xmlDocGetRootElement(doc);
		csvColumnNames = (char**)malloc(sizeof(char*) * 200);
		xmlToCsv(root_element);
		int i = 0;
		while(i < csvColumnNumber){ // Column names are written to csv file.
			strcat(csvFile, csvColumnNames[i++]);
			if(i < csvColumnNumber) strcat(csvFile, ",");
			else strcat(csvFile, "\n");
		}
		xmlToCsv(root_element);
		csvFile[strlen(csvFile) - 1] = '\n';
		fprintf(fp,"%s", csvFile);
		fclose(fp);
	}
	if (strstr(argv[3],  "3") != NULL) { // xml to json operation
		xmlDoc  *doc = NULL;
		xmlNode *root = NULL;
		doc = xmlReadFile(argv[1],NULL,0);
		if (doc != NULL) {
			root = xmlDocGetRootElement(doc);
			List objList; List arrList;
			createList(&objList); createList(&arrList);
			json_object *jsonObject = json_object_new_object();
			elementCounter = findElementSize(root);
			json_object *obj[elementCounter];
			json_object *arr[elementCounter];

			for(int i = 0; i < elementCounter; i++){
				arr[i] = json_object_new_array();
				obj[i] = json_object_new_object();
			}
			xmlNode *temp = root;
			while(temp->type != 1){
				temp = temp->next;
			}
			if(temp->properties != NULL || temp->children != NULL){
				json_object *obj = json_object_new_object();
				json_object_object_add(jsonObject, temp->name, obj);
				addList(&objList, obj, temp->name, temp);
				if(temp->properties != NULL){
					xmlAttr *_xmlAttribute = NULL;
					for(_xmlAttribute = temp->properties; _xmlAttribute; _xmlAttribute = _xmlAttribute->next){
						if(_xmlAttribute->type == 2) {
							json_object_object_add(obj, _xmlAttribute->name, json_object_new_string(_xmlAttribute->children->content));
						}
					}
				}
			}else{
				json_object_object_add(jsonObject, temp->name, json_object_new_string(temp->children->content));
			}
			xmlToJson(findNode(temp->children), &objList, obj, &arrList, arr);
			json_object_to_file_ext(argv[2], jsonObject, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
			xmlFreeDoc(doc);
		}else{
			printf("File does not exist !");
		}
		xmlCleanupParser();
	}
	if (strstr(argv[3],  "4") != NULL) {  /* json to xml operation*/
		xmlDocPtr document = xmlNewDoc(BAD_CAST "1.0");
		json_object *jsonObject = json_object_from_file(argv[1]);
		char *root = strtok(argv[1],".");
		xmlNodePtr rootNode = xmlNewNode(NULL, BAD_CAST root);
		xmlDocSetRootElement(document, rootNode);
		JsonParser(jsonObject, rootNode);
		xmlSaveFormatFileEnc(argv[2], document, "UTF-8", 1);
		xmlFreeDoc(document);
		xmlCleanupParser();
		xmlMemoryDump();
	}
	if (strstr(argv[3],  "5") != NULL) { /*cvs to json operation*/
		csvToJSON(argv[1],argv[2]);
	}
	if (strstr(argv[3],  "7") != NULL) { /*Xml validation opretaion*/
		xmlDocPtr doc;
		xmlSchemaPtr schema = NULL;
		xmlSchemaParserCtxtPtr ctxt;
		char *XMLFileName = argv[1];
		char *XSDFileName = argv[2];

		xmlLineNumbersDefault(1);
		ctxt = xmlSchemaNewParserCtxt(XSDFileName); //create an xml schemas parse context
		schema = xmlSchemaParse(ctxt); //parse a schema definition resource and build an internal XML schema
		xmlSchemaFreeParserCtxt(ctxt); //free the resources associated to the schema parser context
		doc = xmlReadFile(XMLFileName, NULL, 0); //parse an XML file
		if (doc == NULL){
       		 fprintf(stderr, "Could not parse %s\n", XMLFileName);
   		}else{
 		 xmlSchemaValidCtxtPtr ctxt;  //structure xmlSchemaValidCtxt, not public by API
        int ret;
        ctxt = xmlSchemaNewValidCtxt(schema); //create an xml schemas validation context
        ret = xmlSchemaValidateDoc(ctxt, doc); //validate a document tree in memory
        if (ret == 0){
            printf("%s validates\n", XMLFileName);
        }
        else if (ret > 0){
            printf("%s fails to validate\n", XMLFileName);
        }
        else {
            printf("%s validation generated an internal error\n", XMLFileName);
        }
        xmlSchemaFreeValidCtxt(ctxt); //free the resources associated to the schema validation context
        xmlFreeDoc(doc);
		if(schema != NULL)
        xmlSchemaFree(schema); //deallocate a schema structure

		xmlSchemaCleanupTypes(); //cleanup the default xml schemas types library
		xmlCleanupParser(); //cleans memory allocated by the library itself
		xmlMemoryDump(); //memory dump
		}
	}

	return (0);
}

/*createList, create inital list sturcture */
void createList(List *list){
	list->head = (Node*)malloc(sizeof(Node));
	list->head->previous = NULL;
	list->head->next = NULL;
	list->tail = list->head;
	list->nodeIndex = 0;
}
/*addList, Adding new node to list.*/
void addList(List *list, json_object *jobj,const xmlChar *name, xmlNode *node){
	if (list -> nodeIndex == 0) {
		list->head->json_object = jobj;
		list->head->name = name;
		list->head->node = node;
		list->nodeIndex++;
		return;
	}
	Node *newNode = (Node*)malloc(sizeof(Node));
	newNode->json_object = jobj;
	newNode->name = name;
	newNode->node = node;

	list->tail->next = newNode;
	newNode->previous = list->tail;
	list->tail = newNode;
	list->tail->next = NULL;
	list->nodeIndex++;
}

/*findElementSize, Find element size for array and object arrays and  check node type for inc then call by self recursively finalley, return int type size */
int findElementSize(xmlNode *node){
	xmlNode *tempNode = NULL;
	for (tempNode = node; tempNode; tempNode = tempNode->next){
		if (tempNode->type == 1) elementCounter++;
		findElementSize(tempNode->children);
	}
	return elementCounter;
}

/*csvGetLine, read file and split by ',\n\r' line*/
int csvGetLine(FILE *fin){
    int fieldSize;
    char *p, *q;
    if (fgets(buf, sizeof(buf), fin) == NULL)
        return -1;
    fieldSize = 0;
    for (q = buf; (p = strtok(q, ",\n\r")) != NULL; q = NULL)
	 content[fieldSize++] = p;
    return fieldSize;
}

/*Trim space char at string*/
char *trimWhitespace( char * string ){
  char *end;
  while(isspace((unsigned char)*string)) string++;

  if(*string == 0)
    return string;

  end = string + strlen(string) - 1;
  while(end > string && isspace((unsigned char)*end)) end--;

  end[1] = '\0';
  return string;
}

void csvToXML(const char *filename, const char *out_name){
    FILE *fp;
    fp = fopen(filename, "r");								 /* just read file*/
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");     			 /* document pointer */
    xmlNodePtr root = xmlNewNode(NULL, BAD_CAST "xml:root"); /* node pointers */
    xmlDocSetRootElement(doc, root);

    int newf, prevNewf = 0;
    prevNewf = csvGetLine(fp);
    char *_node[prevNewf];									/*_node keep collumn headers*/

	for(int x = 0; x < prevNewf; x++){						/*for fistline,field space keep from memory*/
		_node[x] = (char *) malloc(strlen(content[x]) * sizeof(char));
		strcpy(_node[x],content[x]);						/*then assign contents to _node*/
	}

    if (true) {
        while ((newf = csvGetLine(fp)) != -1) {				/*get field contents*/
            xmlNodePtr node = xmlNewChild(root, NULL, BAD_CAST "xml:row", NULL); /* Add new child node to root*/
            for (int i = 0; i < newf; i++) {
                xmlNewProp(node, BAD_CAST trimWhitespace(_node[i]), BAD_CAST trimWhitespace(content[i])); /*and assign content*/
            }
        }
    } else {
        while ((newf = csvGetLine(fp)) != -1) {
            for (int i = 0; i < newf; i++) {
                xmlNewChild(root, NULL, BAD_CAST trimWhitespace(_node[i]), BAD_CAST trimWhitespace(content[i]));
            }
        }
    }
    fclose(fp);
    xmlSaveFormatFileEnc(out_name, doc, "UTF-8", 1); /*W+ to file acc to out_name parameters, last parameter pretty argument*/
    xmlFreeDoc(doc);								 /* Doc Free from memory*/
    xmlCleanupParser();
}

void csvToJSON(const char *filename, const char *out_name){
	FILE *fp;
    fp = fopen(filename, "r");
    int newf, prevNewf = 0;
    prevNewf = csvGetLine(fp);

    char *_node[prevNewf];												/*_node keep collumn headers*/

    for (int k = 0; k < prevNewf; ++k) {
        _node[k] = (char *) malloc(strlen(content[k]) * sizeof(char));	 /*for fist line,field space keep from memory*/
        strcpy(_node[k], content[k]);
    }

    json_object *root = json_object_new_object();		/*First create root json node json object type*/
    json_object *jsonArray = json_object_new_array();	/*acreate json array type array*/

    while ((newf = csvGetLine(fp)) != -1) {				/*get field contents*/
        json_object *jobj = json_object_new_object();	/*create new json object type object*/
        for (int i = 0; i < newf; i++) {
            json_object *jstr = json_object_new_string(trimWhitespace(content[i])); /* and assign json string */
            json_object_object_add(jobj, trimWhitespace(_node[i]), jstr);	/*then json string add to json object*/
        }
		 json_object_array_add(jsonArray, jobj);	/*json object add to json array*/
    }
    json_object_object_add(root, (const char *) "root", jsonArray); /*finally json array add to root object*/
    json_object_to_file_ext(out_name, root, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY); /*W+ to file acc to out_name parameters*/
    fclose(fp);
}

json_object* getJsonObject(List *list, const xmlChar *name){
	Node *temp = list->head;
	json_object *jsonObject = NULL;
	if (list->nodeIndex == 0) {				 	/*if list is null*/
		return NULL;}

	while(temp != NULL){						/*compare name and temp node if names not same get json object*/
		if (strcmp(temp->name,name) == 0) {
			jsonObject = temp ->json_object;}
		temp = temp -> next;}
	return jsonObject;
}

xmlNode* findNode(xmlNode *xmlnode){  					 /*if there is a type below the given node, it returns it.*/
	xmlNode *temp = NULL;
	for(temp = xmlnode; temp; temp = temp -> next){
		if (temp->type == 1) {
			return temp;
		}
	}
	return NULL;
}

int isThereSibling(List *list, const xmlChar *name, xmlNode *node){
	Node *_currentNode = list->head;  	/*if the given list is not empty, we will make a comparison from the beginning of the list. if they're tied to the nodes, they're brothers and sisters, so we're making the meet accordingly..*/			
	if(list->nodeIndex == false)
		 return 0;
	while(_currentNode != NULL){
		if (strcmp(_currentNode-> name, name) == 0) {
			xmlNode *_tempNode;
			for(_tempNode = _currentNode->node->prev; _tempNode; _tempNode = _tempNode->prev){	/*look previous nodes*/
				if(_tempNode == node) return 1;}
			for(_tempNode = _currentNode->node->next; _tempNode; _tempNode = _tempNode->next){  /*look next nodes*/
				if(_tempNode == node) return 1;}
		}
		_currentNode = _currentNode->next;
	}
	return 0;
}
/*return whether the given node is an arral or an object. if it's Array, it wiil return 1. the comparison operation is performed according to the previous and next nodes of the given node. controls the type of nodes previous and next.*/
int isArray(xmlNode *node){
	xmlNode *temp = NULL;

	for(temp = node->prev; temp; temp = temp->prev){
		if(temp->type == 1 && strcmp(node->name, temp->name) == 0) return 1;
	}

	for(temp = node->next; temp; temp = temp->next){
		if(temp->type == 1 && strcmp(node->name, temp->name) == 0) return 1;
	}

	return 0;
}

void xmlToJson(xmlNode *node, List *obj_list, json_object *json_obj[], List *object_arr, json_object *json_arr[]){
	xmlNode *_currentNode = NULL;
	for(_currentNode = node; _currentNode; _currentNode = _currentNode->next){	
		if (_currentNode->type == 2 || _currentNode->type == 1){	
			if(_currentNode->properties != NULL || findNode(_currentNode->children) != NULL){ /*if there is child node and attr  */ 
				if(getJsonObject(obj_list, (findNode(_currentNode->parent))->name) != NULL && findNode(_currentNode->parent) != NULL){	/*if the node has Parent and parent is not empty*/
					if(isArray(_currentNode) == true){	//array operation
						if(isThereSibling(object_arr, _currentNode->name, _currentNode) == false || getJsonObject(object_arr, _currentNode->name) == NULL){	/* if the node has no sibling nodes 																																					and has namespace so if it's a json array */
							json_object_array_add(json_arr[jsonArrayCounter], json_obj[objectCounter]);	/*the value is added to the JSON array. the object list is also added according to the 																							key and value of node parent. finally object array value and key add to list and counter increment one*/
							json_object_object_add(getJsonObject(obj_list, (findNode(_currentNode->parent))->name), _currentNode->name, json_arr[jsonArrayCounter]);
							addList(object_arr, json_arr[jsonArrayCounter], _currentNode->name, _currentNode);
							jsonArrayCounter++;
						}else{ /* if the node has sibling, add this node and value add json array */
							json_object_array_add(getJsonObject(object_arr, _currentNode->name), json_obj[objectCounter]);
						}
					}else {	/*The node isn't array, in the list given as parameters, node parent key and value values are added as objects. */
						json_object_object_add(getJsonObject(obj_list, (findNode(_currentNode->parent))->name), _currentNode->name, json_obj[objectCounter]);
					}
					/*if the node has not parent or parent is empty, add to list. so if it is json object */
					addList(obj_list, json_obj[objectCounter], _currentNode->name, _currentNode);
					objectCounter++;
				}
			}else { /* if he doesn't have child node and node attr. the rest of the processes continue the same way.*/
				if(isArray(_currentNode) == true){
					if(isThereSibling(object_arr, _currentNode->name, _currentNode) == false || getJsonObject(object_arr, _currentNode->name) == NULL){
						json_object_array_add(json_arr[jsonArrayCounter], json_object_new_string(_currentNode->children->content));
						json_object_object_add(getJsonObject(obj_list, (findNode(_currentNode->parent))->name), _currentNode->name, json_arr[jsonArrayCounter]);
						addList(object_arr, json_arr[jsonArrayCounter], _currentNode->name, _currentNode);
						jsonArrayCounter++;
					}else{
						json_object_array_add(getJsonObject(object_arr, _currentNode->name), json_object_new_string(_currentNode->children->content));
					}
				}	else {
					json_object_object_add(getJsonObject(obj_list, (findNode(_currentNode->parent))->name), _currentNode->name, json_object_new_string(_currentNode->children->content));
				}
			}
			/*if type is not 2 or 1 and it have attr */
			if(_currentNode->properties != NULL){
				xmlAttr *_attribute = NULL;
				for(_attribute = _currentNode->properties; _attribute; _attribute = _attribute->next){
					if(_attribute->type == 2){
						json_object_object_add(getJsonObject(obj_list, _currentNode->name), _attribute->name, json_object_new_string( _attribute->children->content));
					}
				}
			}
		}
		xmlToJson(_currentNode->children, obj_list, json_obj, object_arr, json_arr);
	}
}

/* Jsonparser parse each json object and array and according to key value add to xml */
void JsonParser(json_object *jsonObject, xmlNode *xmlnode){
	enum json_type jsonType;
	json_object_object_foreach(jsonObject, key, value){
		jsonType = json_object_get_type(value);
		if(jsonType == json_type_object) {
			xmlNode* node = xmlNewChild(xmlnode, NULL, key, NULL);
			JsonParser(json_object_object_get(jsonObject, key), node); /* if the node is object return recursively*/
		}else if (jsonType == json_type_array){JsonParserArray(jsonObject, key, xmlnode);
        }else { xmlNewChild(xmlnode, NULL, key, json_object_get_string(value));} /*add to new child to xmlnode*/
	}
}

/*if have json array json parser call jsonParserArray. It */
void JsonParserArray(json_object *jsonObject, char *key, xmlNode *xmlnode){
	json_object *jsonArray = json_object_object_get(jsonObject, key);

	for(int i = 0; i < json_object_array_length(jsonArray); i++){ /*iterates through the array*/
		json_object *jsonValue = json_object_array_get_idx(jsonArray, i);

		if (json_object_get_type(jsonValue) == json_type_object){ /*if its a object, call json parser for object, json parser add  parse object and add xmlNode*/
			xmlNode* node = xmlNewChild(xmlnode, NULL, key, NULL);
			JsonParser(jsonValue, node);
		}else if (json_object_get_type(jsonValue) == json_type_array){
            JsonParserArray(jsonValue, NULL, xmlnode); /*if it's array, call self by recursively and parse array*/
        }else {
            xmlNewChild(xmlnode, NULL, key, json_object_get_string(jsonValue));}
	}
}

static char** csvGetRow(){
	int i = 0;
	char *row = (char*)malloc(sizeof(char) * 5000);
	char **rowColumns = (char**)malloc(sizeof(char*) * 200);
	if(strcmp(csvFile, "") != 0){
		while(csvFile[i] != '\n'){
			row[i] = csvFile[i];
			i++;
		}

		if(row[i - 1] == 13) row[i - 1] = '\0';
		else row[i] = '\0';

		csvFile = csvFile + i + 1;

		char *column = (char*)malloc(sizeof(char) * 500);
		int k = 0,t = 0,z = 0;

		for(t = 0 ; t < strlen(row) ; t++){
			while(row[t] != ',' && t < strlen(row)){
				column[z++] = row[t++];
			}
			column[z] = '\0';
			rowColumns[k] = (char*)malloc(sizeof(char) * 200);
			strcpy(rowColumns[k], column);
			z = 0; k++;
		}
	}

	return rowColumns;
}

static csvRow* csvParseFile(){
	char **columns = csvGetRow();
	csvRow * csv_root = (csvRow*)malloc(sizeof(csvRow));
	csvRow * csv_temp = csv_root;

	while(columns[0] != NULL){
		csv_root->columns = columns;
		columns = csvGetRow();
		if(columns[0] != NULL) csv_root->next = (csvRow*)malloc(sizeof(csvRow));
		csv_root = csv_root->next;
	}

	return csv_temp;
}


static void xmlToCsv(xmlNode *xml_node){ 
	xmlAttr *attribute = NULL;
   xmlNode *cur_node = NULL;
	char* str = NULL;
  for (cur_node = xml_node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
    	attribute = cur_node->properties;
    	while(attribute){
				str = (char*)malloc(sizeof(char) * 200);
				strcpy(str, attribute->name);
				while(strcmp(csvColumnNames[columnCount % csvColumnNumber], str) != 0){
					strcat(csvFile, ",");
					columnCount++;
					if(columnCount % csvColumnNumber == 0){
						csvFile[strlen(csvFile) - 1] = '\0';
						strcat(csvFile, "\n");
					}
				}
				strcat(csvFile, trim(attribute->children->content,0));
				strcat(csvFile, ",");
				columnCount++;
				if(columnCount % csvColumnNumber == 0){
					csvFile[strlen(csvFile) - 1] = '\0';
					strcat(csvFile, "\n");
				}
        attribute = attribute->next;
      }
    }
    xmlToCsv(cur_node->children);
  }
}
