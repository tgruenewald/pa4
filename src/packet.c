/*
 * packet.c
 *
 *  Created on: Nov 24, 2013
 *      Author: terryg
 *      lsof -nP | grep TCP
 *
 */
#include "packet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "const.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset() */
#include <time.h> /* select() */
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#define NIPQUAD(addr) \
         ((unsigned char *)&addr)[0], \
         ((unsigned char *)&addr)[1], \
         ((unsigned char *)&addr)[2], \
         ((unsigned char *)&addr)[3]

	// first, load up address structs with getaddrinfo():
char *createFileKey(struct FileItem *file)
{
	char key[BUFMAX];
	sprintf(key, "%s:%s", file->clientName, file->fileName);
	return key;
}


struct Packet create_packet(char *type, char *from, char *msg, int bytesToSend)
{
	return create_packet_with_more(type,from,"",msg,bytesToSend);
}
struct Packet create_packet_with_more(char *type, char *from, char *more, char *msg, int bytesToSend)
{
    struct Packet packet;
    memset(&packet, 0, sizeof(struct Packet));
    memset(packet.message, '\0', BUFMAX);
    strncpy(packet.type, type, sizeof(packet.type));
    strncpy(packet.from, from, sizeof(packet.from));
    strncpy(packet.areThereMore,more, sizeof(packet.areThereMore));
    memcpy(packet.message, msg,bytesToSend);
    sprintf(packet.length,"%d", strlen(packet.message));
    return packet;
}
void print_packet(struct Packet packet)
{
	printf("Type = [%s]\n", packet.type);
	printf("From = [%s]\n", packet.from);
	printf("Length = [%s]\n", packet.length);
	printf("Message = [%s]\n", packet.message);

	if (strcmp(packet.type, LS_TYPE) == 0 || strcmp(packet.type, REGISTER_FILE_TYPE) == 0)
	{
		struct Files recvFiles;
		memcpy(&recvFiles, packet.message, sizeof(struct Files));
//            char *output = str2md5(packet.message, BUFMAX);
//            printf("server md5 hash %s\n", output);
		int i = 0;
		int fileCount = atoi(recvFiles.numFiles);
		for (i = 0; i < fileCount; i++)
		{
			printf("   File:  %s,  size  %s,  client %s\n",
					recvFiles.files[i].fileName,
					recvFiles.files[i].fileSize,
					recvFiles.files[i].clientName);
		}
	}
}

struct Packet send_and_recv_packet(char *ip, char *port, struct Packet packet)
{
	printf("Sending to %s on port %s\n",ip,port);
//	struct addrinfo hints, *res;
//	int sockfd;
//	// first, load up address structs with getaddrinfo():
//	memset(&hints, 0, sizeof hints);
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//
//    getaddrinfo(ip, port, &hints, &res);
//	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	socklen_t sin_len                 = sizeof(struct sockaddr);

	int rc = connect(sockfd, res->ai_addr, res->ai_addrlen);
	printf("connect status.rc = %d, errno= %d %s\n",rc,errno, strerror(errno));
	if (rc != 0 && errno != EISCONN)
	{
		printf("Connection failed. %d %s\n",errno, strerror(errno));
		char *msg = "Connection failed";
		return create_packet(REJECT_TYPE,"",msg,strlen(msg));
	}

	rc = send(sockfd, &packet, sizeof(struct Packet), 0);  // todo:  if any of these rc's return 1, I need to reconnect and reregister the client with the new port
	printf("send status.rc = %d, errno= %d %s\n",rc,errno, strerror(errno));
	// report back our port and ip
	struct sockaddr_in ouraddr;
    int errcode = getsockname (sockfd, (struct sockaddr *)&ouraddr, &sin_len);
    if (errcode == -1) {
	perror ("getsockname() failed");
	exit(0);
    }

    printf ("Source IP:   %d.%d.%d.%d\n", NIPQUAD(ouraddr.sin_addr.s_addr));
    printf ("Source Port: %d\n", ntohs(ouraddr.sin_port));


	printf("Receiving data back\n");
	struct Packet recvPacket;
	rc = recv(sockfd, &recvPacket, sizeof(struct Packet), 0);
	printf("recv status.rc = %d, errno= %d %s\n",rc,errno, strerror(errno));
	if (rc < 0)
	{
		printf("bad rc = %d\n",rc);
		exit(1);
	}
	print_packet(recvPacket);

	return recvPacket;
}

/*
 *
Terrys-MacBook-Pro:eclipse terryg$ cpp -dM /usr/include/errno.h | grep 'define E' | sort -n -k 3 | grep 56
#define EISCONN 56
Terrys-MacBook-Pro:eclipse terryg$
 */
void send_and_recv_packets(char *ip, char *port, struct Packet packet, struct LinkedList **recvPackets)
{

	printf("Sending to %s on port %s\n",ip,port);
		int rc = connect(sockfd, res->ai_addr, res->ai_addrlen);
		printf("Connection status. %d %s\n",errno, strerror(errno));
		if (rc != 0 && errno != EISCONN)
		{
			printf("Connection failed. %d %s\n",errno, strerror(errno));
			return;
		}
		send(sockfd, &packet, sizeof(struct Packet), 0);


		struct sockaddr_in ouraddr;
		socklen_t sin_len                 = sizeof(struct sockaddr);
	    int errcode = getsockname (sockfd, (struct sockaddr *)&ouraddr, &sin_len);
	    if (errcode == -1) {
		perror ("getsockname() failed");
		exit(0);
	    }

	    printf ("Source IP:   %d.%d.%d.%d\n", NIPQUAD(ouraddr.sin_addr.s_addr));
	    printf ("Source Port: %d\n", ntohs(ouraddr.sin_port));

	printf("Receiving data back\n");
	do
	{
		struct Packet *recvPacket = malloc(sizeof (struct Packet));
		int rc = recv(sockfd, recvPacket, sizeof(struct Packet), 0);
		add_item(recvPackets, "", recvPacket);
		if (rc < 0)
		{
			printf("bad rc = %d\n",rc);
			exit(1);
		}
		print_packet(*recvPacket);
		if (recvPacket->areThereMore[0] == 'n')
			break;
	}
	while (1);
}
