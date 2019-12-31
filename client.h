
#ifndef _CLIENT_H
#define _CLIENT_H

#define MAX_MSG_LEN 4096
#define PROTOCOL_LEN 5

/*
 * Validates command line arguments for client.
 */
void validate(int argc, char *argv[]);

/*
 * Retrieves socket address struct for client.
 */
struct sockaddr_in get_addr(char *host, char *port);

/*
 * Send a message to another socket
 */
void send_msg(int sock, char *msg);

/*
 * Reads input into a "String" and int pointer.
 * Length includes '\0' terminating character.
 */
void read_input(char **msg); 

/*
 * Interprets error messages from the server, if applicable
 */
void interpret_response(char *response);

#endif
