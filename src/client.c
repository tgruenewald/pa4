/*
 * client.c
 *
 *  Created on: Nov 24, 2013
 *      Author: terryg
 *      http://www.planetoid.org/technical/samples/getsockname/getsockname.c
 *
 */
#include "client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
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
#include "LinkStatePacket.h"
#include "Routing.h"
#include "const.h"
#include "packet.h"
#include <assert.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <time.h> /* select() */
#include "str2md5.h"

#define NIPQUAD(addr) \
         ((unsigned char *)&addr)[0], \
         ((unsigned char *)&addr)[1], \
         ((unsigned char *)&addr)[2], \
         ((unsigned char *)&addr)[3]

#define BACKLOG 10     // how many pending connections queue will hold
void setup_socket_for_getting_files(char *port, char *ip)
{
	struct addrinfo hints, *res;
    int opt = 1;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me

	int rc = getaddrinfo(NULL, "0", &hints, &res); // port 0 will get an available port
	if (rc != 0)
	{
		perror("getaddrinfo");
		exit(1);
	}

	// make a socket, bind it, and listen on it:

	incommingFileGetSockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(incommingFileGetSockFd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(1);
    }
	bind(incommingFileGetSockFd, res->ai_addr, res->ai_addrlen);
	listen(incommingFileGetSockFd, BACKLOG);

	// now determine the ip and port
	struct sockaddr_in ouraddr;
	socklen_t sin_len                 = sizeof(struct sockaddr);
    int errcode = getsockname (incommingFileGetSockFd, (struct sockaddr *)&ouraddr, &sin_len);
    if (errcode == -1) {
	perror ("getsockname() failed");
	exit(0);
    }

    sprintf(ip, "%d.%d.%d.%d", NIPQUAD(ouraddr.sin_addr.s_addr));
    sprintf(port, "%d", ntohs(ouraddr.sin_port));

}
int send_file()
{
	int ret = 0;
    fd_set rfds;
    struct timeval tv;
    int new_socket;
    int  activity;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	addr_size = sizeof their_addr;

    //clear the socket set
    FD_ZERO(&rfds);

    //add master socket to set
    FD_SET(incommingFileGetSockFd, &rfds);
    //wait for an activity on one of the sockets
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    activity = select( incommingFileGetSockFd + 1 , &rfds , NULL , NULL , &tv);

    if ((activity < 0) && (errno!=EINTR))
    {
        printf("select error");
    }

    //If something happened on the master socket , then its an incoming connection
    if (FD_ISSET(incommingFileGetSockFd, &rfds))
    {
    	ret = 1;
    	printf("Accepting new connection\n");
        if ((new_socket = accept(incommingFileGetSockFd, (struct sockaddr *) &their_addr, &addr_size))<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // now send the file, probably should fork here
        printf("examing the request to share my file\n");
    	int rc = 0;
    	struct Packet packet;
    	rc = recv(new_socket, &packet, sizeof(struct Packet), 0);
    	if (rc < 0)
    	{
    		printf("bad rc = %d\n",rc);
    		exit(1);
    	}
    	if (rc == 0)
    	{
    		printf("rc = %d, errno= %d %s\n",rc,errno, strerror(errno));
    		return ret;
    	}
    	printf("Recvd\n");
    	print_packet(packet);
		rc = send(new_socket, &packet, sizeof(struct Packet), 0);
		if (rc == 0)
		{
			printf("rc = %d, errno= %d %s\n",rc,errno, strerror(errno));
			return ret;
		}
    	close(new_socket);
    }

    return ret;
}
void start_client(char *clientName, char *serverIp, char *serverPort)
{
    char command[BUFMAX];
    char clientFileListeningPort[LEN];
    char clientFileListeningIp[LEN];
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(serverIp, serverPort, &hints, &res);
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//    struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_CLIENT_TYPE, clientName, clientName));
//
//    if (strcmp(packet.type, REJECT_TYPE) == 0)
//    {
//    	printf("Client %s already registered.  Exiting.  Try a different name.\n", clientName);
//    	exit(0);
//    }

    // setup socket for getting files
    memset(clientFileListeningPort, '\0', sizeof(clientFileListeningPort));
    memset(clientFileListeningIp, '\0', sizeof(clientFileListeningIp));
    setup_socket_for_getting_files(clientFileListeningPort, clientFileListeningIp);

    // register client
	struct Client client;
	memset(&client,'\0',sizeof(struct Client));
	strlcpy(client.clientName, clientName, sizeof(client.clientName));
	strlcpy(client.clientIp, clientFileListeningIp, sizeof(client.clientIp));
	strlcpy(client.clientPort, clientFileListeningPort, sizeof(client.clientPort));
	char buf[BUFMAX];
	memset(buf, '\0', BUFMAX);
	memcpy(buf,  &client, sizeof(client));
	struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_CLIENT_TYPE, clientName, buf));
    if (strcmp(packet.type, REJECT_TYPE) == 0)
    {
    	printf("Client %s already registered.  Exiting.  Try a different name.\n", clientName);
    	exit(0);
    }


    char *tracker;
    char *sep = " ";

    struct LinkedList *mfl = NULL;

    fd_set rfds;
    struct timeval tv;
    int retval;
    int redoPrompt = 1;
    do
    {
        memset(command,'\0',sizeof(command));
        /* Watch stdin (fd 0) to see when it has input. */
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (redoPrompt)
        {
        	redoPrompt = 0;
        	printf("> ");
        	fflush(stdout);
        }
        retval = select(1, &rfds, NULL, NULL, &tv);

        if (retval == -1){
            perror("select()");
            exit(EXIT_FAILURE);
        }
        else if (retval){
             /* FD_ISSET(0, &rfds) is true so input is available now. */

             /* Read data from stdin using fgets. */

             fgets(command, sizeof(command), stdin);
             if (strlen(command) == 0)
             	continue;
             command[strlen(command) -1] = '\0'; // strip off the newline
        }
        if (strlen(command) > 0)  // then the user typed a command
        {
        	redoPrompt = 1;
			printf("%s\n", command);
			char *cmd = strtok_r(command, sep, &tracker);
			if (cmd == NULL)
				continue;

			if (strcmp(cmd, "get") == 0)
			{
				char fileToGet[FILE_NAME_LEN];
				char *arg = strtok_r(NULL, sep, &tracker);
				if (arg != NULL)
				{
					strlcpy(fileToGet, arg, sizeof(fileToGet));
				}

				// find the file in the list
				struct LinkedList *start = mfl;
				while (start != NULL)
				{
					if (strcmp(fileToGet, ((struct FileItem *) start->data)->fileName ) == 0)
					{
						break;
					}
					start = start->next;
				}

				printf("Getting file from:  \n");
				if (start != NULL)
				{
					printf("%-20s   %10s   %-5s    %-20s   %8s\n",
							((struct FileItem *) start->data)->fileName,
							((struct FileItem *) start->data)->fileSize,
							((struct FileItem *) start->data)->clientName,
							((struct FileItem *) start->data)->clientIp,
							((struct FileItem *) start->data)->clientPort);

					// todo: connect to peer client and get file
					struct LinkedList *recvPackets = NULL;
					struct addrinfo hints, *res;
					memset(&hints, 0, sizeof hints);
					hints.ai_family = AF_UNSPEC;
					hints.ai_socktype = SOCK_STREAM;

				    getaddrinfo(((struct FileItem *) start->data)->clientIp,
				    		((struct FileItem *) start->data)->clientPort, &hints, &res);
					int clientSockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//					send_and_recv_packets(clientSockfd,
//							((struct FileItem *) start->data)->clientIp,
//					        ((struct FileItem *) start->data)->clientPort,
//					        create_packet("get", clientName, ((struct FileItem *) start->data)->fileName ),
//					        &recvPackets);


						int rc = connect(clientSockfd, res->ai_addr, res->ai_addrlen);
						printf("Connection status. %d %s\n",errno, strerror(errno));
						if (rc != 0 && errno != EISCONN)
						{
							printf("Connection failed. %d %s\n",errno, strerror(errno));
							return;
						}
						struct Packet packet = create_packet(GET_TYPE, clientName, ((struct FileItem *) start->data)->fileName );
						send(clientSockfd, &packet, sizeof(struct Packet), 0);

					printf("Receiving data back\n");
					int fileSize = atoi(((struct FileItem *) start->data)->fileSize);
					char *fileContentsBuffer = malloc(fileSize);

					do
					{
						struct Packet recvPacket;
						int rc = recv(clientSockfd, &recvPacket, sizeof(struct Packet), 0);
						if (rc < 0)
						{
							printf("bad rc = %d\n",rc);
							exit(1);
						}

						if (fileSize < 0)
						{
							fileSize = atoi(recvPacket.length);
							fileContentsBuffer = malloc(fileSize);
						}


						if (recvPacket.areThereMore[0] == 'n')
							break;
					}
					while (1);
				}

			}
			if (strcmp(cmd, "ls") == 0)
			{
				struct LinkedList *recvPackets = NULL;
				send_and_recv_packets(serverIp, serverPort,
						create_packet(LS_TYPE, clientName, ""), &recvPackets);

				struct LinkedList *start = recvPackets;
				while (start != NULL)
				{

					struct Files *recvFiles = malloc(sizeof(struct Files));
					memcpy(recvFiles,  ((struct Packet *) start->data)->message, sizeof(struct Files));
		//            char *output = str2md5(packet.message, BUFMAX);
		//            printf("server md5 hash %s\n", output);
					int i = 0;
					int fileCount = atoi(recvFiles->numFiles);
					for (i = 0; i < fileCount; i++)
					{
						printf("Got : %s\n", recvFiles->files[i].fileName);
						struct FileItem *file = malloc(sizeof(struct FileItem));
						strlcpy(file->clientName, recvFiles->files[i].clientName, sizeof(file->clientName));
						strlcpy(file->fileName, recvFiles->files[i].fileName, sizeof(file->fileName));
						strlcpy(file->fileSize, recvFiles->files[i].fileSize, sizeof(file->fileSize));
						strlcpy(file->clientIp, recvFiles->files[i].clientIp, sizeof(file->clientIp));
						strlcpy(file->clientPort, recvFiles->files[i].clientPort, sizeof(file->clientPort));
						add_item(&mfl, createFileKey(file), file);
					}


					start = start->next;
				}
				// todo:  need to clean up this linked list;

				start = mfl;
				while (start != NULL)
				{
					printf("%-20s   %10s   %-5s    %-20s   %8s\n",
							((struct FileItem *) start->data)->fileName,
							((struct FileItem *) start->data)->fileSize,
							((struct FileItem *) start->data)->clientName,
							((struct FileItem *) start->data)->clientIp,
							((struct FileItem *) start->data)->clientPort);
					start = start->next;
				}
			}
			if (strcmp(cmd, "send") == 0)
			{
				// from http://www.lemoda.net/c/list-directory/index.html
				// send files to the server
				DIR * d;
				char * dir_name = ".";

				/* Open the current directory. */

				d = opendir (dir_name);

				if (! d) {
					fprintf (stderr, "Cannot open directory '%s': %s\n",
							 dir_name, strerror (errno));
					exit (EXIT_FAILURE);
				}

				struct Files clientFiles;
				memset(&clientFiles, '\0', sizeof(clientFiles));
				int i = 0;
				while (1)
				{
					struct dirent * entry;

					entry = readdir (d);
					if (! entry) {
						break;
					}
					if (entry->d_type == DT_REG && i < MAX_FILE_COUNT)
					{
						struct stat st;
						stat(entry->d_name, &st);
						printf ("sending %s, size = %d\n", entry->d_name, (int) st.st_size);
						strlcpy(clientFiles.files[i].clientName, clientName, sizeof(clientFiles.files[i].clientName));
						strlcpy(clientFiles.files[i].fileName, entry->d_name, sizeof(clientFiles.files[i].fileName));
						sprintf(clientFiles.files[i].fileSize, "%d",(int) st.st_size);

						i++;
					}

					if (i == MAX_FILE_COUNT)
					{
						sprintf(clientFiles.numFiles,"%d", i);
						i = 0;
						// now send the files and start over.
						char buf[BUFMAX];
						memset(buf, '\0', BUFMAX);
						memcpy(buf,  &clientFiles, sizeof(clientFiles));
						struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_FILE_TYPE, clientName, buf));
						memset(&clientFiles, '\0', sizeof(clientFiles));
					}
				}
				/* Close the directory. */
				if (closedir (d)) {
					fprintf (stderr, "Could not close '%s': %s\n",
							 dir_name, strerror (errno));
					exit (EXIT_FAILURE);
				}

				if (i > 0)
				{
					// then there were a few more to send
					printf("A few more to send\n");
					sprintf(clientFiles.numFiles,"%d", i);
					i = 0;
					// now send the files and start over.
					char buf[BUFMAX];
					memset(buf, '\0', BUFMAX);
					memcpy(buf,  &clientFiles, sizeof(clientFiles));
					struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_FILE_TYPE, clientName, buf));
					memset(&clientFiles, '\0', sizeof(clientFiles));
				}

	//            char buf[BUFMAX];
	//            memset(buf, '\0', BUFMAX);
	//            memcpy(buf,  &clientFiles, sizeof(clientFiles));
	//                    char *output = str2md5(buf, BUFMAX);
	//                    printf("client md5 hash %s\n", output);
	//            // todo:  remove debugging code
	//			struct Files *recvFiles = malloc(sizeof(struct Files));
	//			memcpy(recvFiles, buf, sizeof(struct Files));
	////			int i = 0;
	//			for (i = 0; i < 3; i++)
	//			{
	//				printf("Got : %s\n", recvFiles->files[i].fileName);
	//			}
	//
	//            // now send the files
	//            struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_FILE_TYPE, clientName, buf));

			}
			if (strcmp(cmd, "reg") == 0)
			{
				printf("Registering client\n");
				char *arg = strtok_r(NULL, sep, &tracker);
				if (arg != NULL)
				{
					clientName = arg;
				}
				struct Client client;
				memset(&client,'\0',sizeof(struct Client));
				strlcpy(client.clientName, clientName, sizeof(client.clientName));
				strlcpy(client.clientIp, clientFileListeningIp, sizeof(client.clientIp));
				strlcpy(client.clientPort, clientFileListeningPort, sizeof(client.clientPort));
				char buf[BUFMAX];
				memset(buf, '\0', BUFMAX);
				memcpy(buf,  &client, sizeof(client));
				struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_CLIENT_TYPE, clientName, buf));
			}

			if (strcmp(cmd, "unreg") == 0)
			{
				printf("Unregistering client\n");
				char *arg = strtok_r(NULL, sep, &tracker);
				if (arg != NULL)
				{
					clientName = arg;
				}
				struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(UNREGISTER_CLIENT_TYPE, clientName, clientName));
			}
        } // end command processing
        else
        {
        	// look to see if we need to pick up a file
        	if (send_file())
        	{
        		redoPrompt = 1;
        	}

        }
/*
   		 FD_ZERO(&select_fds);
   		 FD_SET(sd, &select_fds);

   		 timeout.tv_sec = 0;
   		 timeout.tv_usec = 50000; // 50ms timeout

   	// set timeout to 50ms
   		 if (select(sd + 1, &select_fds, NULL,NULL, &timeout) != 0)
   		 {

   		 }
   		 */

    }
    while (strcmp("exit", command) != 0);

	freeaddrinfo(res);
	close(sockfd);

    exit(0);
}
