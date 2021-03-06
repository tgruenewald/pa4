/*
 * server.c
 *
 *  Created on: Nov 24, 2013
 *      Author: terryg
 */
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* memset() */
#include <time.h> /* select() */
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include "packet.h"
#include "const.h"
#include <assert.h>
#include <signal.h>
#include <errno.h>

// http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
void start_server(char *port)
{
    int opt = 1;
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i ,  sd;
    int max_sd;
//    struct sockaddr_in address;


    //set of socket descriptors
    fd_set readfds;

    clients = NULL;  // keyed by client name.
    mfl = NULL;  // master file list.  keyed by filename

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }


    ////
	struct sockaddr_storage their_addr;
	struct addrinfo hints, *res;
	socklen_t addr_size;

	addr_size = sizeof their_addr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me

	getaddrinfo(NULL, port, &hints, &res);

	// make a socket, bind it, and listen on it:

	master_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
	bind(master_socket, res->ai_addr, res->ai_addrlen);
	listen(master_socket, BACKLOG);
    ////


    //accept the incoming connection
//    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while(1)
    {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++)
        {
            //socket descriptor
            sd = client_socket[i];

            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);

            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

        if ((activity < 0) && (errno!=EINTR))
        {
            printf("select error");
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
        	printf("Accepting new connection\n");
            new_socket = accept(master_socket, (struct sockaddr *) &their_addr, &addr_size);
            if (new_socket < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection\n");
//            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(their_addr.sin_addr) , ntohs(their_addr.sin_port));
        	if (handle_connection(new_socket) != 0)
            {
                //Somebody disconnected , get his details and print
//                getpeername(new_socket , (struct sockaddr*)&their_addr , (socklen_t*)&addr_size);
//                printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                printf("host disconnected\n");
                //Close the socket and mark as 0 in list for reuse
                close( new_socket );
                client_socket[i] = 0;
            }

//            //send new connection greeting message
//            if( send(new_socket, message, strlen(message), 0) != strlen(message) )
//            {
//                perror("send");
//            }
//
//            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
//            printf("looping through existing connection %d\n", i);
            if (FD_ISSET( sd , &readfds))
            {
            	printf("Handling existing connection\n");
            	if (handle_connection(sd) != 0)
                {
                    //Somebody disconnected , get his details and print
//                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
//                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            		printf("Host disconnected\n");
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                }
            }
        }
    }
}

int handle_connection(int new_fd)
{
	int rc = 0;
	struct Packet packet;
	rc = recv(new_fd, &packet, sizeof(struct Packet), 0);
	if (rc < 0)
	{
		printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
		exit(1);
	}
	if (rc == 0)
	{
		printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
		return 1;
	}
	printf("Recvd\n");
	print_packet(packet);
	if (strcmp(packet.type,LS_TYPE) == 0)
	{
		// this will need to send back multiple packets.
		struct LinkedList *start = mfl;
		struct Files clientFiles;
		int i = 0;
		int totalCount = 0;
		int listSize = listsize(mfl);
		char areThereMore[YN_LEN];
		memset(&clientFiles, '\0', sizeof(clientFiles));
		while (start != NULL)
		{
			strncpy(clientFiles.files[i].clientName, ((struct FileItem *) start->data)->clientName , sizeof(clientFiles.files[i].clientName));
			strncpy(clientFiles.files[i].fileName,   ((struct FileItem *) start->data)->fileName, sizeof(clientFiles.files[i].fileName));
			strncpy(clientFiles.files[i].fileSize,   ((struct FileItem *) start->data)->fileSize, sizeof(clientFiles.files[i].fileSize));
			struct LinkedList *client = find(clients, clientFiles.files[i].clientName);
			if (client == NULL)
			{
				printf("ERROR:  Unable to find client %s\n", clientFiles.files[i].clientName);
				exit(1);// todo:  not sure if i should do this
			}
			else
			{
				strncpy(clientFiles.files[i].clientPort, ((struct Client *) client->data)->clientPort, sizeof(clientFiles.files[i].clientPort));
				strncpy(clientFiles.files[i].clientIp, ((struct Client *) client->data)->clientIp, sizeof(clientFiles.files[i].clientIp));
			}
			i++;
			totalCount++;

			if (i == MAX_FILE_COUNT)
			{
				// look ahead to see if more files exist
				if (totalCount < listSize)
				{
					strcpy(areThereMore, "y");
				}
				else
				{
					strcpy(areThereMore, "n");
				}


				sprintf(clientFiles.numFiles,"%d", i);
				i = 0;
				// now send the files and start over.
				char buf[BUFMAX];
				memset(buf, '\0', BUFMAX);
				memcpy(buf,  &clientFiles, sizeof(clientFiles));
				struct Packet p = create_packet_with_more(LS_TYPE, "",areThereMore, buf,BUFMAX);
				rc = send(new_fd, &p, sizeof(struct Packet), 0);
				if (rc == 0)
				{
					printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
					return 1;
				}
				memset(&clientFiles, '\0', sizeof(clientFiles));
			}

			start = start->next;
		}

		if (i > 0)
		{
			// then send the last files
			strcpy(areThereMore, "n");
			sprintf(clientFiles.numFiles,"%d", i);
			i = 0;
			// now send the files and start over.
			char buf[BUFMAX];
			memset(buf, '\0', BUFMAX);
			memcpy(buf,  &clientFiles, sizeof(clientFiles));
			struct Packet p = create_packet_with_more(LS_TYPE, "", areThereMore, buf,BUFMAX);
			rc = send(new_fd, &p, sizeof(struct Packet), 0);
			if (rc == 0)
			{
				printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
				return 1;
			}
			memset(&clientFiles, '\0', sizeof(clientFiles));
		}
	}
	if (strcmp(packet.type,REGISTER_FILE_TYPE) == 0)
	{
		printf("Registering files\n");

		struct Files *recvFiles = malloc(sizeof(struct Files));
		memcpy(recvFiles, packet.message, sizeof(struct Files));
//            char *output = str2md5(packet.message, BUFMAX);
//            printf("server md5 hash %s\n", output);
		int i = 0;
		int fileCount = atoi(recvFiles->numFiles);
		int haveFilesChanged = 0;
		for (i = 0; i < fileCount; i++)
		{

			printf("Got : %s\n", recvFiles->files[i].fileName);
			struct FileItem *file = malloc(sizeof(struct FileItem));
			strncpy(file->clientName, recvFiles->files[i].clientName, sizeof(file->clientName));
			strncpy(file->fileName, recvFiles->files[i].fileName, sizeof(file->fileName));
			strncpy(file->fileSize, recvFiles->files[i].fileSize, sizeof(file->fileSize));

			// look to see if this file already exist
			struct LinkedList *prev = find(mfl, createFileKey(file));
			if (prev != NULL)
			{
				// then look to see if it needs to be updated
				if (strcmp( ((struct FileItem *) prev->data)->fileSize, file->fileSize ) != 0)
				{
					haveFilesChanged = 1;
					printf("Updateing file size of %s\n", file->fileName);
					strncpy(((struct FileItem *) prev->data)->fileSize, file->fileSize, sizeof(((struct FileItem *) prev->data)->fileSize) );
				}
				free(file);

			}
			else
			{
				// add new file to the list
				add_item(&mfl,createFileKey(file), file );
				haveFilesChanged = 1;
			}

			print_list("File list:", mfl);
		}

		if (haveFilesChanged)
		{
			resend_updated_file_list();
		}


//			// first check if already registered
//			struct LinkedList *prevClient = find(clients, packet.message);
//			if (prevClient != NULL)
//			{
//				strncpy(packet.type, REJECT_TYPE, sizeof packet.type);
//			}
//			else
//			{
//				// then new client, so add to list.
//				struct Client *client = malloc(sizeof(struct Client));
//				memcpy(client, packet.message, sizeof(struct Client));
//				//strncpy(client->clientName, packet.message, packet.length);
//				add_item(&clients,client->clientName, client);
//				strncpy(packet.type, OK_TYPE, sizeof packet.type);
//				print_list("Client list:", clients);
//			}
	}

	if (strcmp(packet.type,REGISTER_CLIENT_TYPE) == 0)
	{
		printf("Registering client\n");
		struct Client *client = malloc(sizeof(struct Client));
		memset(client, '\0', sizeof(struct Client));
		memcpy(client, packet.message, sizeof(struct Client));
		memset(client->clientIp, '\0', sizeof(client->clientIp));
		get_client_ip(new_fd, client->clientIp);  // http://www.beej.us/guide/bgnet/output/html/multipage/getpeernameman.html
		printf("client port = %s, ip = %s\n", client->clientPort, client->clientIp);
		// first check if already registered
		struct LinkedList *prevClient = find(clients, client->clientName);
		if (prevClient != NULL)
		{
			strncpy(packet.type, REJECT_TYPE, sizeof packet.type);
			free(client);
		}
		else
		{
			// then new client, so add to list.
			add_item(&clients,client->clientName, client);
			strncpy(packet.type, OK_TYPE, sizeof packet.type);
			print_list("Client list:", clients);
		}
	}

	if (strcmp(packet.type,UNREGISTER_CLIENT_TYPE) == 0)
	{
		printf("Unegistering client\n");

		// first check if already registered
		struct LinkedList *prevClient = find(clients, packet.message);
		if (prevClient == NULL)
		{
			strncpy(packet.type, REJECT_TYPE, sizeof packet.type);
		}
		else
		{
			// then remove existing client from list
			struct Client *freeMe = (struct Client *) remove_item(&clients, ((struct Client *) prevClient->data)->clientName);
			free(freeMe);
			strncpy(packet.type, OK_TYPE, sizeof packet.type);
			print_list("Client list after unregister:", clients);

			// now remove any files that the client has
			struct LinkedList *start = mfl;
			while (start != NULL)
			{
				if ( strcmp( ((struct FileItem *) start->data)->clientName, packet.message) == 0)
				{
					// then remove this file
					struct FileItem *freeFile = (struct FileItem *) remove_item(&mfl,createFileKey((struct FileItem *) start->data) );
					free(freeFile);

					print_list("File list:", mfl);
				}
				start = start->next;
			}

			resend_updated_file_list();
		}
	}


	printf("Sending packet back to client\n");
	rc = send(new_fd, &packet, sizeof(struct Packet), 0);
	if (rc == 0)
	{
		printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
		return 1;
	}
	printf("Sent.\n");
	return 0;
}
void resend_updated_file_list()
{
	struct LinkedList *start = clients;
	while (start != NULL)
	{
		{
			struct addrinfo hints, *res;
			memset(&hints, 0, sizeof hints);
			hints.ai_family = AF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			printf("Sending updated ls to %s ip = %s, port = %s\n",
					((struct Client *) start->data)->clientName,
					((struct Client *) start->data)->clientIp,
					((struct Client *) start->data)->clientPort);

		    getaddrinfo(((struct Client *) start->data)->clientIp,
		    		((struct Client *) start->data)->clientPort, &hints, &res);
			int clientSockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

				int rc = connect(clientSockfd, res->ai_addr, res->ai_addrlen);
				printf("Connection status. %d %s\n",errno, strerror(errno));
				if (rc != 0 && errno != EISCONN)
				{
					printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
					return;
				}
				char buf[BUFMAX];
				memset(buf,'\0', BUFMAX);
				strcpy(buf,"ls");
				struct Packet packet = create_packet(LS_TYPE, ((struct Client *) start->data)->clientName, buf,BUFMAX);
				rc = send(clientSockfd, &packet, sizeof(struct Packet), 0);
				printf("Connection status. %d %s\n",errno, strerror(errno));
				if (rc == -1 && errno != EISCONN)
				{
					printf("ERROR:  rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
					return;
				}
				close(clientSockfd);
	            freeaddrinfo(res);
		}
		start = start->next;
	}
}
void get_client_ip(int s, char *client_ip)
{
	// assume s is a connected socket

	socklen_t len;
	struct sockaddr_storage addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;

	len = sizeof addr;
	getpeername(s, (struct sockaddr*)&addr, &len);

	// deal with both IPv4 and IPv6:
	if (addr.ss_family == AF_INET) {
	    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
	    port = ntohs(s->sin_port);
	    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
	    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
	    port = ntohs(s->sin6_port);
	    inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	printf("Peer IP address: %s\n", ipstr);
	strcpy(client_ip, ipstr);
}



