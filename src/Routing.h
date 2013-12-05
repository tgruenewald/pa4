/*
 * Routing.h
 *
 *  Created on: Nov 14, 2013
 *      Author: terryg
 */
#include "const.h"
#include "LinkedList.h"
#include "LinkStatePacket.h"
#ifndef ROUTING_H_
#define ROUTING_H_

struct Route
{
	char node[LEN];
	int cost;
	char next_hop[LEN];
	int isRootNeighbor;
};

struct RoutingTable
{
	struct LinkedList *confirmed;  // list of Route
	struct LinkedList *tentative;  // list of Route
	struct LinkedList *holding;    // list of LSPs that don't yet connect to anything in tentative or confirmed
};
void print_table(struct LinkedList *table);
void fprint_table(FILE *fp, struct LinkedList *table);
struct LSP *find_root(struct LinkedList *holding);
struct LSP *is_neighbor(struct LSP *root, struct LSP *lsp);
void _add_route2(FILE *fpointer, struct RoutingTable *table, struct LSP *lsp, char *from);
void add_route(FILE *fp, struct RoutingTable *table, struct LSP *lsp, char *from);
struct LinkedList *find_next_hop(struct LinkedList *table, char *next_hop);
void print_all(struct RoutingTable *table);
struct Route *create_route(char *node, int cost, char *next_hop, int isRootNeighbor);
int compare_route(struct Route *r1, struct Route *r2);
int compare_routing_tables(struct LinkedList *table1, struct LinkedList *table2);
struct LinkedList *clone_routing_table(struct LinkedList *orig);
#endif /* ROUTING_H_ */
