#include "const.h"
#include <stdlib.h>
#include <stdio.h>
#include "LinkStatePacket.h"
#include "LinkedList.h"
#include <string.h>
#include <assert.h>


void combine(struct LSP *recvLsp, struct LSP *curLsp)
{

	// find it in the current
	struct LinkedList *a = find(curLsp->all,recvLsp->id);
	if (a != NULL)
	{
		// only add it if it is more current than what I have already
		if (((struct LSP *) a->data)->seq < recvLsp->seq)
		{
			a->data = delete_lsp_node((struct LSP *) a->data);
			a->data = recvLsp;
		}
	}
	else
	{
		// we don't have this entry, so add it to the list
		curLsp->all = list_add(curLsp->all, recvLsp->id, recvLsp);
	}
}

/*
  my old understanding of how I wanted combine to work

  // returns 1 if the passed in LSP should be deleted, 0 if it should be kept.
int combine(struct LSP *recvLsp, struct LSP *curLsp)
{
	int shouldDeleteLspAfterwards = 1;
	if (curLsp->all == NULL)
	{
		curLsp->all = recvLsp->nearby;
		recvLsp->nearby = NULL;

		// add the head node (possibly) to the list
		shouldDeleteLspAfterwards = add_lsp(curLsp->all,recvLsp);
	}
	else
	{
		// add the head node (possibly) to the list
		shouldDeleteLspAfterwards = add_lsp(curLsp->all,recvLsp);

		// walk through every node in the received's neighbor list
		struct LinkedList *n = recvLsp->nearby;
		while (n != NULL)
		{
			struct LinkedList *a = NULL;

			// find it in the current
			a = find(curLsp->all,((struct LSP *) n->data)->id);
			if (a != NULL)
			{
				// only add it if it is more current than what I have already
				if (((struct LSP *) a->data)->seq < ((struct LSP *) n->data)->seq)
				{
					a->data = delete_lsp_node((struct LSP *) a->data);
					a->data = n->data;
					n->data = NULL;
				}
			}
			else
			{
				// we don't have this entry, so add it to the list
				list_add(curLsp->all,((struct LSP *) n->data)->id, n->data);
				n->data = NULL;
			}

			n = n->next;
		}
	}

	return shouldDeleteLspAfterwards;
}
 */

/*
 * add a new lsp to the existing list.  The passed in list must not be NULL
 * returns 1 if the passed in lsp should be deleted
 * returns 0 otherwise
 */
int add_lsp(struct LinkedList *list, struct LSP *lsp)
{
	int shouldDeleteLspAfterwards = 1;

	struct LinkedList *a = NULL;
	a = find(list,lsp->id);
	if (a != NULL)
	{
		// only add it if it is more current than what I have already
		if (((struct LSP *) a->data)->seq < lsp->seq)
		{
			a->data = delete_lsp_node((struct LSP *) a->data);
			a->data = lsp;
			shouldDeleteLspAfterwards = 0; // since it was added to the list
			print_list("add_lsp after find and replace",list);
		}
	}
	else
	{
		print_list("add_lsp before list_add",list);
		list = list_add(list,lsp->id, lsp);
		print_list("add_lsp after list_add",list);
		shouldDeleteLspAfterwards = 0; // since it was added to the list
	}

	return shouldDeleteLspAfterwards;
}

struct LSP *delete_lsp_node(struct LSP *lsp)
{
	// I assume that there will be nothing in this node
//	assert(lsp->all == NULL);
//	assert(lsp->nearby == NULL); // todo:  put this back -- valgrind doesn't like it
	pthread_mutex_lock(&lock);
	free(lsp);
	pthread_mutex_unlock(&lock);
	return NULL;
}

//void print_list(struct LinkedList *list)
//{
//	struct LinkedList *p = list;
//	printf("[");
//	while (p != NULL)
//	{
//		printf("%s ", p->key);
//		p = p->next;
//	}
//	printf("]\n");
//}

void serialize_lsp(char *buf, struct LSP *lsp)
{

	memset(buf,'\0',BUFMAX);
	sprintf(buf,"%s:%s:%d:",lsp->id,lsp->myPort,lsp->seq);
	struct LinkedList *p = lsp->nearby;
	while (p != NULL)
	{
		char temp[BUFMAX];
		sprintf(temp,"%s,%s,%s,%d;",((struct LSP *) p->data)->id, ((struct LSP *) p->data)->myPort,((struct LSP *) p->data)->parentPort,((struct LSP *) p->data)->cost);
		strcat(buf,temp);
		p = p->next;
	}
}

struct LSP *deserialize_lsp(char *pbuf)
{
	char buf[BUFMAX];
	memset(buf,'\0',BUFMAX);
	strcpy(buf,pbuf);
	char *back;

	pthread_mutex_lock(&lock);
	struct LSP *lsp = malloc(sizeof(struct LSP));
	pthread_mutex_unlock(&lock);
	memset(lsp,0,sizeof(struct LSP));
	lsp->all = NULL;
	lsp->nearby = NULL;
	char *token;
	token = strtok_r(buf,":", &back);
	while (token != NULL)
	{
		strcpy(lsp->id,token);
		token = strtok_r(NULL,":", &back);
		strcpy(lsp->myPort,token);
		token = strtok_r(NULL,":", &back);
		lsp->seq = atoi(token);
		token  = strtok_r(NULL,",", &back);

		lsp->nearby = NULL;
		while (token != NULL)
		{
			pthread_mutex_lock(&lock);
			struct LSP *neighbor = malloc(sizeof(struct LSP));
			pthread_mutex_unlock(&lock);
			memset(neighbor,0,sizeof(struct LSP));
			neighbor->all = NULL;
			neighbor->nearby = NULL;

			strcpy(neighbor->id,token);
			token  = strtok_r(NULL,",", &back);
			strcpy(neighbor->myPort,token);
			token  = strtok_r(NULL,",", &back);
			strcpy(neighbor->parentPort,token);
			token  = strtok_r(NULL,";", &back);
			neighbor->cost = atoi(token);


			if (lsp->nearby == NULL)
			{
				// first one, so connect to the lsp
				lsp->nearby = list_add(NULL,neighbor->id,neighbor);
			}
			else
			{
				lsp->nearby = list_add(lsp->nearby,neighbor->id,neighbor);
			}
			token  = strtok_r(NULL,",", &back); // get the next id
		}


	}

	return lsp;
}

void print_lsp_list(struct LinkedList *start)
{
	struct LinkedList *p = start;
	while (p != NULL)
	{

		print_lsp((struct LSP *) p->data);
		p = p->next;
	}
	printf("------------------------\n");
}
void print_lsp(struct LSP *lsp)
{
	fprint_lsp(stdout, lsp);
}
void fprint_lsp(FILE *fp, struct LSP *lsp)
{
	struct LinkedList *n = lsp->nearby;
	fprintf(fp, "LSP:  %s\n", lsp->id);
	while (n != NULL)
	{
		fprintf(fp, "  neighbor: %s cost %d\n", ((struct LSP *) n->data)->id, ((struct LSP *) n->data)->cost);
		n = n->next;
	}
}

struct LSP *convert_to_lsp(struct LinkedList *lsp_list)
{
	struct LSP *lsp = NULL;
	struct LSP *friend = NULL;
	print_list("convert to lsp",lsp_list);
	struct LinkedList *p = lsp_list;
	int i = 0;
	while (p != NULL)
	{
		i++; // todo:  remove this code later
		if (i > 10000)
		{
			printf("BAD!!!  Breaking out of infinite loop\n");
			break;
		}
		if (lsp == NULL)
		{
			pthread_mutex_lock(&lock);
			lsp = malloc(sizeof(struct LSP));
			pthread_mutex_unlock(&lock);
			memset(lsp,0,sizeof(struct LSP));
			strcpy(lsp->id,((struct InitialState *) p->data)->src);
			strcpy(lsp->myPort,((struct InitialState *) p->data)->srcPort);
			lsp->cost = 0;
			lsp->seq = 0;
			lsp->nearby = NULL;
			lsp->all = NULL;
		}

		pthread_mutex_lock(&lock);
		friend = malloc(sizeof(struct LSP));
		pthread_mutex_unlock(&lock);
		memset(friend,0,sizeof(struct LSP));
		friend->nearby = NULL;
		friend->all = NULL;
		strcpy(friend->id,((struct InitialState *) p->data)->dest);
		printf("friend->id = [%s]\n",friend->id);
		strcpy(friend->myPort,((struct InitialState *) p->data)->destPort);
		strcpy(friend->parentPort,((struct InitialState *) p->data)->srcPort);
		friend->cost = atoi(((struct InitialState *) p->data)->cost);
		friend->seq = 0;
		print_list("convert_to_lsp before list add",lsp_list);
		printf("lsp_list address = %p, lsp->nearby address = %p\n", lsp_list,lsp->nearby);
		lsp->nearby = list_add(lsp->nearby,friend->id,friend);
//		struct LinkedList *t = list_add(NULL,friend->id,friend);
		print_list("convert_to_lsp after list add",lsp_list);

		p = p->next;

	}

	return lsp;
}

struct LinkedList *parseNeighbors(char *initFile, char *id)
{
	struct LinkedList *neighbors = NULL;
	   FILE *fp;
		if ((fp = fopen(initFile, "r")) == NULL)
		{
			printf("Unable to open log file %s\n", initFile);
			exit(1);
		}
	char buf[BUFMAX + 1];
	while (fgets(buf,BUFMAX,fp) != 0)
	{
		//printf("Line:  %s\n",buf);
		pthread_mutex_lock(&lock);
		struct InitialState *lsp = malloc(sizeof(struct InitialState));
		pthread_mutex_unlock(&lock);
		memset(lsp,0,sizeof(struct LSP));
		if (sscanf(buf,"<%[^','],%[^','],%[^','],%[^','],%[^'>']>",lsp->src,lsp->srcPort,lsp->dest,lsp->destPort,lsp->cost) == 5)
		{
			if (strcmp(lsp->src,id) == 0)
			{
				printf("Adding %s\n", lsp->dest);
				neighbors = list_add(neighbors,lsp->dest,(struct InitialState *) lsp);
			}
			else
			{
				pthread_mutex_lock(&lock);
				free(lsp);
				pthread_mutex_unlock(&lock);
			}

		}
		else
		{
			pthread_mutex_lock(&lock);
			free(lsp);
			pthread_mutex_unlock(&lock);
			printf("Error parsing line [%s]\n", buf);
			exit(1);
		}
	}


	fclose(fp);

	return neighbors;
}

void cleanupLinkStatePacket(struct LinkedList *lsp)
{
	delete_list(lsp);
}

void cleanLSP(struct LSP *lsp)
{
//	struct LinkedList *n = lsp->nearby;
//	while (n != NULL)
//	{
//		struct LinkedList *next = n->next;
//		cleanLSP((struct LSP *) n->data);
//		free(n);
//		n = next;
//	}
	pthread_mutex_lock(&lock);
	struct LinkedList *a = lsp->all;
	while (a != NULL)
	{
		struct LinkedList *next = a->next;
		cleanLSP((struct LSP *) a->data);
		free(a);
		a = next;
	}

	free(lsp);
	pthread_mutex_unlock(&lock);
}
