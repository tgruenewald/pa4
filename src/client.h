/*
 * client.h
 *
 *  Created on: Nov 24, 2013
 *      Author: terryg
 */

#ifndef CLIENT_H_
#define CLIENT_H_
void start_client(char *clientName, char *serverIp, char *serverPort);
void setup_socket_for_getting_files(char *port, char *ip);
int send_file(char *clientName, char *serverIp, char *serverPort);
void ls(char *clientName, char *serverIp, char *serverPort);
void send_file_list(char *clientName, char *serverIp, char *serverPort);
struct LinkedList *mfl;
#endif /* CLIENT_H_ */
