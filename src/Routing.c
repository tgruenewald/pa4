/*
 * Routing.c
 *
 *  Created on: Nov 14, 2013
 *      Author: terryg
 */
#include "Routing.h"
#include "LinkedList.h"
#include "LinkStatePacket.h"
#include <assert.h>
#include <string.h> /* memset() */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
struct Route *create_route(char *node, int cost, char *next_hop, int isRootNeighbor)
{
	struct Route *entry = malloc(sizeof(struct Route));
	strcpy(entry->node, node);
	entry->cost = cost;
	strcpy(entry->next_hop,next_hop);  // '-' indicates that this is the node the table is for
	entry->isRootNeighbor = isRootNeighbor;
	return entry;
}

// performs a deep copy
struct LinkedList *clone_routing_table(struct LinkedList *orig)
{
	struct LinkedList *p = orig;
	struct LinkedList *new = NULL;
	while (p != NULL)
	{
		new = list_add(new,((struct Route *) p->data)->node, create_route( ((struct Route *) p->data)->node,
		((struct Route *) p->data)->cost,
		((struct Route *) p->data)->next_hop,
		((struct Route *) p->data)->isRootNeighbor));
		p = p->next;
	}
	return new;
}

// compare just the confirmed.  returns 0 on that they are the same, 1 if they are different
int compare_routing_tables(struct LinkedList *table1, struct LinkedList *table2)
{
	struct LinkedList *t1 = table1;
	struct LinkedList *other_table = table2;
	while (t1 != NULL)
	{

		struct LinkedList *t2 = find(other_table, ((struct Route *) t1->data)->node);
		if (t2 == NULL)
		{
			return 1;
		}
		if (compare_route( (struct Route *) t1->data,  (struct Route *) t2->data))
		{
			return 1;
		}

		t1 = t1->next;
	}

	// now switch
	t1 = table2;
	other_table = table1;
	while (t1 != NULL)
	{

		struct LinkedList *t2 = find(other_table, ((struct Route *) t1->data)->node);
		if (t2 == NULL)
		{
			return 1;
		}
		if (compare_route( (struct Route *) t1->data,  (struct Route *) t2->data))
		{
			return 1;
		}

		t1 = t1->next;
	}

	return 0;
}

// returns 0 on that they are the same, 1 if they are different
int compare_route(struct Route *r1, struct Route *r2)
{
	if (  strcmp( r1->node, r2->node ) != 0)
	{
		return 1;
	}

	if (  strcmp( r1->next_hop, r2->next_hop ) != 0)
	{
		return 1;
	}

	if ( r1->cost != r2->cost  )
	{
		return 1;
	}
	return 0;
}
void add_route(FILE *fp, struct RoutingTable *table, struct LSP *lsp, char *from)
{
	// Grab a clone for comparison later
	struct LinkedList *oldConfirmed = clone_routing_table(table->confirmed);
	_add_route2(fp,table,lsp,from);

	if (compare_routing_tables(oldConfirmed, table->confirmed))
	{
		if (fp != NULL)
		{
			// then log that there was a change
			time_t now;
			time(&now);
			fprintf(fp, "At %s, LSP coming from %s caused a routing table change\n",ctime(&now), from);
			fprint_lsp(fp,lsp);
			fprint_table(fp,table->confirmed);
			fprintf(fp,"-----------------\n\n");
			fflush(fp);
		}
	}
}
struct LSP *find_root(struct LinkedList *holding)
{
	struct LinkedList *h = holding;
	while (h != NULL)
	{
		if (  ((struct LSP *) h->data)->isRoot)
			return (struct LSP *) h->data;
		h = h->next;
	}

	return NULL;
}

struct LSP *is_neighbor(struct LSP *root, struct LSP *lsp)
{
	struct LinkedList *n = root->nearby;
	while (n != NULL)
	{
		if (  strcmp( ((struct LSP *) n->data)->id, lsp->id) == 0)
			return (struct LSP *) n->data;
		n = n->next;
	}
	return NULL;
}
// internal use only.  call add_route instead of this one.
// this add route will correctly calculate the inbetween route
void _add_route2(FILE *fpointer, struct RoutingTable *table, struct LSP *lsp, char *from)
{
	// only add the lsp if it is new
	struct LinkedList *l = find(table->holding, lsp->id);
	if (l != NULL)
	{

		if (((struct LSP *) l->data)->isRoot)
		{
			printf("no need to handle this lsp\n");
			return;
		}
	}


	printf("Start--\n");
	print_all(table);
	if (strcmp(from,"-") == 0)
	{
		lsp->isRoot = 1;
	}
	else
	{
		lsp->isRoot = 0;
	}

	// clear out confirmed and tentative.  This will need to be rebuilt from
	// what i have learned each time.  All I have is holding.
	table->confirmed = NULL;
	table->tentative = NULL;

	// replace the new lsp in holding
	if (replace(table->holding,lsp->id, lsp) == NULL) // todo:  need to free this memory returned here
	{
		table->holding = list_add(table->holding, lsp->id, lsp);
	}

	// now that the lsp has been added, walk through the holding since it doesn't
	// matter the order which the LSPs arrive.

	// find the root.  This always starts the tentative list
	struct LSP *root = find_root(table->holding);

	if (root == NULL)
	{
		// can't proceed.  this must be found
		return;
	}

	// add root to confirmed
	struct Route *entry = create_route(root->id, 0, "-",1);

	table->confirmed = list_add(table->confirmed,
			entry->node,
			entry);
	struct LSP *nextLsp = root;
	int costToNext = 0;
	char path[LEN];
	memset(path,'\0',LEN);
	while (1)
	{
		printf("next lsp is %s\n", nextLsp->id);
		struct LSP *neighbor = NULL;
		if ( (neighbor = is_neighbor(root, nextLsp)) != NULL)
		{
			// then cost goes back to zero
			costToNext = neighbor->cost;
			printf("costToNext goes back cost from root %d (root is %s, and next is %s)\n",costToNext, root->id, nextLsp->id);
			strcpy(path,nextLsp->id);
		}
		else
		{
			struct LinkedList *entry = find(table->confirmed, nextLsp->id);
			if (entry != NULL)
			{
				printf("Using nextLsp's next hop of %s\n",  ((struct Route *) entry->data)->next_hop);
				strcpy(path, ((struct Route *) entry->data)->next_hop);
			}
		}
		// now find all of nextLsp's neighbors.  Add them to tentative and pick the smallest one.
		struct LinkedList *h = table->holding;
		while (h != NULL)
		{

			if ( (neighbor = is_neighbor(nextLsp, (struct LSP *) h->data)) != NULL)
			{
				if (nextLsp == root)
				{
					strcpy(path,neighbor->id);
				}
				struct Route *entry = create_route(((struct LSP *) h->data)->id,
						neighbor->cost + costToNext,
						path,
						1);

				// replace an entry in here if it cost less than the currently stored entry
				struct LinkedList *prevTentativeRoute = find(table->tentative, entry->node);
				if (prevTentativeRoute != NULL)
				{
					// look to see if my current route is better than previous
					if (((struct Route *) prevTentativeRoute->data)->cost > entry->cost)
					{
						// then replace it
						printf("Replacing previous %s with cost of %d with new route with cost of only %d\n",entry->node,((struct Route *) prevTentativeRoute->data)->cost,entry->cost );
						replace(table->tentative,entry->node, entry);
					}
				}
				else
				{
					if (find(table->confirmed, entry->node) == NULL && prevTentativeRoute == NULL)
					{
						printf("Adding %s to tentative with a cost of %d\n", entry->node, entry->cost);
						table->tentative = list_add(table->tentative,
								entry->node,
								entry);
					}
				}

			}

			h = h->next;
		}

		// now find the lowest cost lsp.   This will be what path we start on.
		printf("Tentative\n");
		print_table(table->tentative);
		printf("---------------------\n");
		struct LinkedList *t = table->tentative;
		struct Route *lowest = NULL;
		while (t != NULL)
		{
			if (find(table->confirmed, ((struct Route *) t->data)->node) == NULL)
			{
				// only look for those that are not on confirmed
				if (lowest == NULL)
				{
					lowest = (struct Route *) t->data;
				}

				if ( ((struct Route *) t->data)->cost <= lowest->cost)
				{
					// choose based on ID if the cost is the same
					if ( ((struct Route *) t->data)->cost == lowest->cost)
					{
						if (strcmp(  ((struct Route *) t->data)->node,   lowest->node) < 0)
						{
							printf( "Assigning to %s since it is lexigraphically less than %s\n",((struct Route *) t->data)->node, lowest->node);
							lowest = (struct Route *) t->data;
						}
					}
					else
					{
						lowest = (struct Route *) t->data;
					}
				}
			}
			t = t->next;
		}

		if (lowest != NULL)
		{
			// add it to confirmed
			costToNext = lowest->cost;
			printf("root: found the lowest:  %s, cost is now %d\n", lowest->node,costToNext);


			struct LinkedList *next = find(table->holding,lowest->node);
			nextLsp = (struct LSP *) next->data;
		}

		if (lowest == NULL)
		{
			// not much can be done.  exiting
			break;
		}

		if (listsize(table->tentative) == 0)
		{
			printf("No more tentative.  Stop\n");
			break;
		}

		// since the LSP was found, I can add it to confirmed
		remove_item(&table->tentative, lowest->node);
		table->confirmed = list_add(table->confirmed,lowest->node, lowest);

//		if (listsize(table->tentative) == 0)
//		{
//			printf("No more tentative.  Stop\n");
//			break;
//		}
	} // end while forever
	printf("End--\n");
	print_all(table);

}



void print_all(struct RoutingTable *table)
{
	printf("Routing table\n");
	print_table(table->confirmed);
	printf("**********************\n");
	printf("Tentative\n");
	print_table(table->tentative);
	printf("**********************\n");
	printf("Holding\n");
	print_lsp_list(table->holding);
	printf("**********************\n\n");
}

struct LinkedList *find_next_hop(struct LinkedList *table, char *next_hop)
{
	struct LinkedList *p = table;
	while (p != NULL)
	{
		if ( strcmp( ((struct Route *) p->data)->next_hop, next_hop) == 0)
		{
			break;
		}
		p = p->next;
	}

	return p;
}
void print_table(struct LinkedList *table)
{
	fprint_table(stdout, table);
}
void fprint_table(FILE *fp, struct LinkedList *table)
{
	struct LinkedList *p = table;
	struct LinkedList *routingTableRoot = find_next_hop(table,"-");
	if (routingTableRoot != NULL)
		fprintf(fp,"Routing table for %s\n", ((struct Route *) p->data)->node);
	fprintf(fp, "[%-10s][%-5s][%-10s]\n","Node", "Cost","NextNode");
	while (p != NULL)
	{
		fprintf(fp,"[%-10s][%5d][%10s]\n", ((struct Route *) p->data)->node, ((struct Route *) p->data)->cost, ((struct Route *) p->data)->next_hop);
		p = p->next;
	}

}
