/*
 * server.h
 *
 *  Created on: Nov 24, 2013
 *      Author: terryg
 */
#include <stdio.h>

#ifndef SERVER_H_
#define SERVER_H_
void start_server(char *port);
void _start_server(char *port); // deprecated
int handle_connection(int new_fd);
void get_client_ip(int s, char *client_ip);
void resend_updated_file_list();

struct LinkedList *clients;  // keyed by client name.
struct LinkedList *mfl;  // master file list.  keyed by filename

#define BACKLOG 10     // how many pending connections queue will hold
#endif /* SERVER_H_ */
