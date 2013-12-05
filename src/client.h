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
int send_file();
#endif /* CLIENT_H_ */
