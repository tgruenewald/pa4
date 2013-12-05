/*
 ===========================================================================
 Name        : routed_LS.c
 Author      : Terry Gruenewald
 Version     : PA4
 Copyright   : Your copyright notice
 Description : Server that simulates link state routing
 Grabbed code snippets from
 http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html

 ============================================================================
 */

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
#include "LinkStatePacket.h"
#include "Routing.h"
#include "const.h"
#include <assert.h>
#include <signal.h>
#include "server.h"
#include "client.h"
char port[LEN];
char clientName[LEN];
char serverIp[LEN];
char serverPort[LEN];

void sig_handler(int signo)
{
  if (signo == SIGINT)
  {

	    printf("received SIGINT\n");
	    printf("closing log file\n");
//	    if (logfile != NULL)
//	    {
//	    	fclose(logfile);
//	    	logfile = NULL;
//	    }
	    printf("Cleanly shutting down\n");
	    exit(0);

  }
}

int main(int argc, char *argv[])
{
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	  printf("\ncan't catch SIGINT\n");
	// check command line args.

//		if (argc == 2)
//		{
//			// then run in routing table mode
//			buildrt(argv[1]);
//			exit(0);
//		}

	if (argc == 2)
	{
		// then running in server mode
		printf("Running in server mode with port %s\n", argv[1]);
		strcpy(port, argv[1]);
		start_server(port);
	}
	else if (argc < 4)
	{
		printf("usage : %s <client name> <server IP> <server port> \n", argv[0]);
		exit(1);
	}

	if (argc == 4)
	{
		printf("Running as client:\n");
		strcpy(clientName, argv[1]);
		strcpy(serverIp, argv[2]);
		strcpy(serverPort, argv[3]);
		start_client(clientName, serverIp, serverPort);
	}
	return 0;


}
