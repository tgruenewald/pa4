/*
 * LinkStatePacket.h
 *
 *  Created on: Nov 3, 2013
 *      Author: terryg
 */
#include "const.h"
#include "LinkedList.h"
#ifndef LINKSTATEPACKET_H_
#define LINKSTATEPACKET_H_


struct LSP
{
	char id[LEN];  // the original node id
	int seq;
	char myPort[LEN];
	char parentPort[LEN];
	int cost;
	struct LinkedList *nearby; // LSP

	// not serialized
	struct LinkedList *all; // LSP -- all known routers
	char receivedFromId[LEN]; // what node this went through.
	int isRoot;

};
struct InitialState
{
	char src[LEN];
	char srcPort[LEN];
	char dest[LEN];
	char destPort[LEN];
	char cost[LEN];
};
void combine(struct LSP *recvLsp, struct LSP *curLsp);
struct LSP *delete_lsp_node(struct LSP *lsp);
int add_lsp(struct LinkedList *list, struct LSP *lsp);
//void print_list(struct LinkedList *list);
void serialize_lsp(char *buf, struct LSP *lsp);
struct LSP *deserialize_lsp(char *buf);
struct LSP *convert_to_lsp(struct LinkedList *lsp_list);
void cleanupLinkStatePacket(struct LinkedList *lsp);
struct LinkedList *parseNeighbors(char *initFile, char *id);
void cleanLSP(struct LSP *lsp);
void print_lsp_list(struct LinkedList *start);
void print_lsp(struct LSP *lsp);
void fprint_lsp(FILE *fp, struct LSP *lsp);
#endif /* LINKSTATEPACKET_H_ */
