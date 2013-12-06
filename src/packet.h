/*
 * packet.h
 *
 *  Created on: Nov 24, 2013
 *      Author: terryg
 */

#ifndef PACKET_H_
#define PACKET_H_
#include "const.h"
#include "LinkedList.h"
#define MAX_FILE_COUNT 3

struct Packet
{
	char type[LEN];
	char from[LEN];
	char length[LEN];
	char areThereMore[YN_LEN];
	char message[BUFMAX];
};

struct Client
{
	char clientName[LEN];
	char clientIp[LEN];
	char clientPort[LEN];
};
struct FileItem
{
	char fileName[FILE_NAME_LEN];
	char fileSize[LEN];
	char clientName[LEN];
	char clientIp[LEN];
	char clientPort[LEN];
};
struct Files
{
	struct FileItem files[MAX_FILE_COUNT];
	char numFiles[20];
};


// global variables
struct addrinfo hints, *res;
int sockfd;

int incommingFileGetSockFd;


#define BACKLOG 10     // how many pending connections queue will hold


#define REGISTER_CLIENT_TYPE "register_client"
#define UNREGISTER_CLIENT_TYPE "unregister_client"
#define REGISTER_FILE_TYPE "register_files"
#define UNREGISTER_FILE_TYPE "unregister_files"
#define LS_TYPE "ls"
#define GET_TYPE "get"

#define REJECT_TYPE "reject"
#define OK_TYPE "ok"

struct Packet create_packet(char *type, char *from, char *msg, int bytesToSend);
struct Packet create_packet_with_more(char *type, char *from, char *more, char *msg,int bytesToSend);
void print_packet(struct Packet packet);
struct Packet send_and_recv_packet(char *ip, char *port, struct Packet packet);
void send_and_recv_packets(char *ip, char *port, struct Packet packet, struct LinkedList **recvPackets);
char *createFileKey(struct FileItem *file);
//void send_packet(char *ip, char *port, struct Packet packet);
//struct Packet recv_packet(char *port);
#endif /* PACKET_H_ */
