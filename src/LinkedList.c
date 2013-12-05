/*
 * LinkedList.c
 *
 *  Created on: Nov 3, 2013
 *      Author: terryg
 */
#include "LinkedList.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
void delete_list(struct LinkedList *start)
{
	pthread_mutex_lock(&lock);
	delete_list_no_lock(start);
	pthread_mutex_unlock(&lock);
}

void delete_list_no_lock(struct LinkedList *start)
{
	while (start != NULL)
	{
		struct LinkedList *temp = start->next;
		if (start->data != NULL)
			free(start->data);
		free(start);
		start = temp;
	}
}

void print_list(char *msg, struct LinkedList *start)
{
	struct LinkedList *p = start;
	printf("%s[",msg);
	int i = 0;
	while (p != NULL)
	{
		if (i > 30)  // todo:  not sure if i should keep or adjust this
		{
			printf("infinite loop detected, breaking");
			break;
		}
		printf("%s ", p->key);
		p = p->next;
	}
	printf("]\n");
}

// deprecated
struct LinkedList *list_add(struct LinkedList *start,char *key, void *data)
{
	struct LinkedList **s = &start;
	add_item(&s, key, data);
	return *s;
}
void add_item(struct LinkedList **start,char *key, void *data)
{
	struct LinkedList *walk = *start;
	struct LinkedList *prev = NULL;
	while (walk != NULL)
	{
		prev = walk;
		walk = walk->next;
	}

	// now at last node
	pthread_mutex_lock(&lock);
	struct LinkedList *temp = malloc(sizeof(struct LinkedList));
	pthread_mutex_unlock(&lock);

	memset(temp,0,sizeof(struct LinkedList));
	strcpy(temp->key,key);
	temp->data = data;
	temp->next = NULL;
	if (prev != NULL)
	{
		prev->next = temp;
	}
	else
	{
		// then this must have been a new list
		prev = temp;
		*start = prev;
	}
}

// finds key, and replaces that data with the passed in data.  Returns the old data
// so it can be freed.  Returns NULL if that key wasn't found
void *replace(struct LinkedList *start, char *key, void *newData)
{
	struct LinkedList *p = find(start,key);
	void *oldData = NULL;
	if (p != NULL)
	{
		oldData = p->data;
		p->data = newData;
	}
	return oldData;
}

// removes key from the linked list.  Returns the data item removed if found, or otherwise null if not found.
// Note:  it will only remove the first item it finds.
// Note:  you must pass in the pointer to the pointer for the start of the list since that start might change.
// struct LinkedList *list = malloc(...)
// usage:  data = remove_item(&list, "B");
void *remove_item(struct LinkedList **start, char *key)
{
	if (start == NULL)
	{
		return NULL;
	}

	if (*start == NULL)
	{
		return NULL;
	}
	void *oldData = NULL;


	struct LinkedList *q = *start;
	struct LinkedList *prev = *start;
	while (q != NULL)
	{
		struct LinkedList *next = q->next;
		if (strcmp(q->key,key) == 0)  // then we found our item.
		{

			if (q == *start)
			{
				// so this was the first one
				*start = next;
				prev = next;
				// then clean up
				oldData = q->data;
				free(q);

				q = next;
				return oldData;
			}
			else
			{
				// so this is a middle node
				prev->next = next;
				prev = next;

				// then clean up
				oldData = q->data;
				free(q);
				q = next;
				return oldData;
			}

		}
		else
		{
			// don't delete
			prev = q;
			q = q->next;
		}

	}
	return oldData;
}

struct LinkedList *find(struct LinkedList *start, char *key)
{
	while (start != NULL)
	{
		if (strcmp(start->key,key) == 0)
		{
			break;
		}
		start = start->next;
	}
	return start;
}

int listsize(struct LinkedList *start)
{
	int size = 0;
	while (start != NULL)
	{
		size++;
		start = start->next;
	}

	return size;
}

