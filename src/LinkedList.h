/*
 * LinkedList.h
 *
 *  Created on: Nov 3, 2013
 *      Author: terryg
 */
#include "const.h"

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_
struct LinkedList
{
	char key[LEN];
	void *data;
	struct LinkedList *next;
};
void print_list(char *msg, struct LinkedList *start);
void delete_list(struct LinkedList *start);
void delete_list_no_lock(struct LinkedList *start);
int listsize(struct LinkedList *start);
struct LinkedList *list_add(struct LinkedList *start,char *key, void *data);
void add_item(struct LinkedList **start,char *key, void *data);
struct LinkedList *find(struct LinkedList *start, char *key);
void *replace(struct LinkedList *start, char *key, void *newData);
void *remove_item(struct LinkedList **start, char *key);
#endif /* LINKEDLIST_H_ */
