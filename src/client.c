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
#include "const.h"
#include "packet.h"
#include <assert.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <time.h> /* select() */


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
    freeaddrinfo(res);

}
int send_file(char *clientName, char *serverIp, char *serverPort)
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

//    	printf("Accepting new connection\n");
    	new_socket = accept(incommingFileGetSockFd, (struct sockaddr *) &their_addr, &addr_size);
        if (new_socket<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
		int rc = 0;
		struct Packet packet;
		rc = recv(new_socket, &packet, sizeof(struct Packet), 0);
		if (rc < 0)
		{
			printf("rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
			close(new_socket);
			exit(1);
		}
		if (rc == 0)
		{
			printf("rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
			close(new_socket);
			exit(0);
		}


		if (strcmp(packet.type, LS_TYPE) == 0)
		{
			close(new_socket);
			ret = 1;
			ls(clientName, serverIp, serverPort);
		}
		else // it must be a file get request
		{
			int pid = fork();
			if (pid < 0)
			{
				perror("Error forking");
				exit(1);
			}

			if (pid == 0)
			{
				// in child process
				close(incommingFileGetSockFd);
				if (strcmp(packet.type, GET_TYPE) == 0)
				{

					// read the file from disk
					FILE *fp = fopen(packet.message, "r");
					fseek(fp, 0L, SEEK_END);
					int fileSize = ftell(fp);
					fseek(fp, 0L, SEEK_SET);

					char *buffer = malloc(fileSize);
					char *startBuf = buffer;
					if (fread(buffer, sizeof(*buffer), fileSize,fp) != fileSize)
					{
						perror("Mismatched read.  Not all bytes were read");
						fclose(fp);
						close(new_socket);
						free(buffer);
						exit(1);
					}
					fclose(fp);
					int bytesSent = 0;
					char buf[BUFMAX];
					while (bytesSent < fileSize)
					{
						char more[YN_LEN];
						memset(buf,'\0', BUFMAX);
						int sent = 0;
						if (bytesSent + BUFMAX > fileSize)
						{
							// then this will be the last packet
							strcpy(more,"n");
		//					printf("Sending last packet:  %d bytes will be sent\n",(fileSize-bytesSent ));
							memcpy(buf,buffer, fileSize-bytesSent);
			//        		memcpy(buf,"123456789012", fileSize-bytesSent);
							//printf("Sending [%s]\n", buf);
							sent = (fileSize-bytesSent);
							bytesSent = bytesSent + sent;
						}
						else
						{
							// still more to go
							strcpy(more,"y");
							memcpy(buf, buffer,BUFMAX);
							sent = BUFMAX;
							bytesSent = bytesSent + sent;
						}


						struct Packet packet = create_packet_with_more(GET_TYPE, "",more, buf,BUFMAX);
						sprintf(packet.length,"%d", sent);
						rc = send(new_socket, &packet, sizeof(struct Packet), 0);
						if (rc == 0)
						{
							printf("rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
							free(startBuf);
							close(new_socket);
							exit(0);
						}

						buffer = buffer + BUFMAX;

					}
					free(startBuf);
				}

				close(new_socket);
				exit(0);
			}  // end child process
			else
			{
				// parent process
				close(new_socket);
			}
		}
    }

    return ret;
}

void send_file_list(char *clientName, char *serverIp, char *serverPort)
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
			strncpy(clientFiles.files[i].clientName, clientName, sizeof(clientFiles.files[i].clientName));
			strncpy(clientFiles.files[i].fileName, entry->d_name, sizeof(clientFiles.files[i].fileName));
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
			struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_FILE_TYPE, clientName, buf,BUFMAX));
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
		struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_FILE_TYPE, clientName, buf,BUFMAX));
		memset(&clientFiles, '\0', sizeof(clientFiles));
	}
}
void ls(char *clientName, char *serverIp, char *serverPort)
{
	delete_list(mfl);
	mfl = NULL;
	struct LinkedList *recvPackets = NULL;
	send_and_recv_packets(serverIp, serverPort,
			create_packet(LS_TYPE, clientName, "",0), &recvPackets);

	struct LinkedList *start = recvPackets;
	while (start != NULL)
	{

		struct Files *recvFiles = malloc(sizeof(struct Files));
		memcpy(recvFiles,  ((struct Packet *) start->data)->message, sizeof(struct Files));
		int i = 0;
		int fileCount = atoi(recvFiles->numFiles);
		for (i = 0; i < fileCount; i++)
		{
			printf("Got : %s\n", recvFiles->files[i].fileName);
			struct FileItem *file = malloc(sizeof(struct FileItem));
			strncpy(file->clientName, recvFiles->files[i].clientName, sizeof(file->clientName));
			strncpy(file->fileName, recvFiles->files[i].fileName, sizeof(file->fileName));
			strncpy(file->fileSize, recvFiles->files[i].fileSize, sizeof(file->fileSize));
			strncpy(file->clientIp, recvFiles->files[i].clientIp, sizeof(file->clientIp));
			strncpy(file->clientPort, recvFiles->files[i].clientPort, sizeof(file->clientPort));
			add_item(&mfl, createFileKey(file), file);
		}


		start = start->next;
	}

	start = mfl;
	printf("%-30s   %10s   %-10s    %-20s   %8s\n",
			"File name",
			"Size",
			"Client",
			"IP",
			"Port");
	printf("%-30s   %10s   %-10s    %-20s   %8s\n",
			"---------------",
			"---------",
			"---------",
			"---------------",
			"---------");
	while (start != NULL)
	{
		if (strlen(((struct FileItem *) start->data)->clientIp) > 0)
		{
		printf("%-30s   %10s   %-5s    %-20s   %8s\n",
				((struct FileItem *) start->data)->fileName,
				((struct FileItem *) start->data)->fileSize,
				((struct FileItem *) start->data)->clientName,
				((struct FileItem *) start->data)->clientIp,
				((struct FileItem *) start->data)->clientPort);
		}
		else
		{
			printf("empty client ip for file %s client %s\n", ((struct FileItem *) start->data)->fileName, ((struct FileItem *) start->data)->clientName);
		}
		start = start->next;
	}
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

    // setup socket for getting files
    memset(clientFileListeningPort, '\0', sizeof(clientFileListeningPort));
    memset(clientFileListeningIp, '\0', sizeof(clientFileListeningIp));
    setup_socket_for_getting_files(clientFileListeningPort, clientFileListeningIp);

    // register client
	struct Client client;
	memset(&client,'\0',sizeof(struct Client));
	strncpy(client.clientName, clientName, sizeof(client.clientName));
	strncpy(client.clientIp, clientFileListeningIp, sizeof(client.clientIp));
	strncpy(client.clientPort, clientFileListeningPort, sizeof(client.clientPort));
	char buf[BUFMAX];
	memset(buf, '\0', BUFMAX);
	memcpy(buf,  &client, sizeof(client));
	struct Packet p = send_and_recv_packet(serverIp, serverPort, create_packet(REGISTER_CLIENT_TYPE, clientName, buf,BUFMAX));
    if (strcmp(p.type, REJECT_TYPE) == 0)
    {
    	printf("Client %s already registered.  Exiting.  Try a different name.\n", clientName);
    	exit(0);
    }

    // send list of files in the current directory to the server
    send_file_list(clientName, serverIp, serverPort);

    char *tracker;
    char *sep = " ";

    mfl = NULL;

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
        //http://stackoverflow.com/questions/6418232/how-to-use-select-to-read-input-from-keyboard-in-c
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
             {
            	 redoPrompt = 1;
             	continue;
             }
             command[strlen(command) -1] = '\0'; // strip off the newline
        }
        if (strlen(command) > 0)  // then the user typed a command
        {
        	redoPrompt = 1;
			printf("%s\n", command);
			char *cmd = strtok_r(command, sep, &tracker);
			if (cmd == NULL)
			{
				redoPrompt = 1;
				continue;
			}

			if (strcmp(cmd, "get") == 0)
			{
				char fileToGet[FILE_NAME_LEN];
				char *arg = strtok_r(NULL, sep, &tracker);
				if (arg != NULL)
				{
					strncpy(fileToGet, arg, sizeof(fileToGet));
				}

				// find the file in the list
				struct LinkedList *start = mfl;
				while (start != NULL)
				{
					if (strcmp(fileToGet, ((struct FileItem *) start->data)->fileName ) == 0 &&
							strcmp(clientName,((struct FileItem *) start->data)->clientName ))  // don't get from yourself. that's silly
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

					struct LinkedList *recvPackets = NULL;
					struct addrinfo hints, *res;
					memset(&hints, 0, sizeof hints);
					hints.ai_family = AF_UNSPEC;
					hints.ai_socktype = SOCK_STREAM;

				    getaddrinfo(((struct FileItem *) start->data)->clientIp,
				    		((struct FileItem *) start->data)->clientPort, &hints, &res);
					int clientSockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);


						int rc = connect(clientSockfd, res->ai_addr, res->ai_addrlen);
						printf("Connection status. %d %s\n",errno, strerror(errno));
						if (rc != 0 && errno != EISCONN)
						{
							printf("rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
							return;
						}
						struct Packet packet = create_packet(GET_TYPE, clientName, ((struct FileItem *) start->data)->fileName, strlen(((struct FileItem *) start->data)->fileName) );
						send(clientSockfd, &packet, sizeof(struct Packet), 0);

					printf("Receiving data back\n");
					int fileSize = atoi(((struct FileItem *) start->data)->fileSize);
					char *fileContentsBuffer = malloc(fileSize);
					char *startOfBuffer = fileContentsBuffer;
					memset(fileContentsBuffer, '\0', fileSize);
					int accumLen = 0;

					do
					{
						if (fileSize > 0)
						{
							float progress = ((float) accumLen/fileSize) * 100.0;
							if (( ((int) progress) % 10) == 0)
							{
								printf("Progress %3.1f%%\n\033[F\033[J", progress);
							}
						}
						struct Packet recvPacket;
						int rc = recv(clientSockfd, &recvPacket, sizeof(struct Packet), 0);
						if (rc < 0)
						{
							printf("rc = %d, errno= %d %s on line %d\n",rc,errno, strerror(errno),__LINE__);
							exit(1);
						}

						memcpy(fileContentsBuffer, recvPacket.message, atoi(recvPacket.length));
						fileContentsBuffer = fileContentsBuffer + atoi(recvPacket.length);
						accumLen = accumLen + atoi(recvPacket.length);

						if (recvPacket.areThereMore[0] == 'n')
							break;
					}
					while (1);
					printf("Progress 100%%\n");
					printf("Writing file to disk\n");
					// now write out the file to disk.
		            FILE *fp = fopen(((struct FileItem *) start->data)->fileName, "w");
		            fwrite(startOfBuffer, sizeof(char), accumLen,fp);
		            fclose(fp);
		            free(startOfBuffer);
		            freeaddrinfo(res);
		            close(clientSockfd);
		            printf("File retrieved!!!\n");
				}

			}
			if (strcmp(cmd, "ls") == 0)
			{
				ls(clientName, serverIp, serverPort);
			}
			if (strcmp(cmd, "send") == 0)
			{
				send_file_list(clientName, serverIp, serverPort);
			}

        } // end command processing
        else
        {
        	// look to see if we need to pick up a file
        	if (send_file(clientName, serverIp,serverPort))
        	{
        		redoPrompt = 1;
        	}

        }

    }
    while (strcmp("exit", command) != 0);

	printf("Unregistering client\n");
	char *arg = strtok_r(NULL, sep, &tracker);
	if (arg != NULL)
	{
		clientName = arg;
	}
	struct Packet packet = send_and_recv_packet(serverIp, serverPort, create_packet(UNREGISTER_CLIENT_TYPE, clientName, clientName,strlen(clientName)));

	freeaddrinfo(res);
	close(sockfd);

    exit(0);
}
