
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include "client.h"

void validate_port(char *str) 
{
    char *ptr;

    int port = strtol(str, &ptr, 10);;
    
    if (port <= 4000) {
		switch (errno)
		{
		case EINVAL:
			printf("Invalid port\n");
			exit(1);
			break;
		
		case ERANGE:
			printf("Port out of range\n");
			exit(1);
			break;

        default:
            printf("Port must be over 4000\n");
			exit(1);
		}
	}
}

/*
 * Validates command line arguments.
 */
void validate(int argc, char *argv[]) 
{
	if (argc != 3) {
		printf("bruh\n");
		exit(1);
	}

	validate_port(argv[2]);
}

struct sockaddr_in get_addr(char *host, char *port) 
{
	struct addrinfo *info;
	struct addrinfo hints = {
		.ai_flags = AI_ADDRCONFIG | AI_V4MAPPED,
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP
	};

	if (getaddrinfo(host, port, &hints, &info) != 0) {
		printf("Failure at getaddrinfo()\n");
		exit(1);
	}

	struct sockaddr_in addr = *(struct sockaddr_in*) (info->ai_addr);

	freeaddrinfo(info);

	return addr;
}

void send_msg(int sock, char *msg)
{
	if (send(sock, msg, strlen(msg) + 1, 0) < 0) {			// send message to server
		printf("Failed to send\n");
		exit(1);
	}
}

/*
 * Reads input into a "String" and int pointer.
 * Length includes '\0' terminating character.
 */
void read_input(char **msg) 
{
	*msg = NULL;							// clear out old data
	size_t n = 0;

	int len = getline(msg, &n, stdin) - 1;	// read from stdin
	if (len < 0) {
		printf("Failure at getline()\n");
		exit(1);
	}

	(*msg)[len] = '\0';					// null terminate message

	// printf("%s %d\n", *msg, len);
}

/*
 * Interprets error messages from the server, if applicable
 */
void interpret_response(char *response)
{
	char *msg = NULL;

	if (strcmp("ER:EXIST", response)  == 0) {
		msg = "This box already exists.";
	}
	else if (strcmp("ER:OPEND", response ) == 0) {
		msg = "This box is currently in use.";
	}
	else if (strcmp("ER:NOTMT", response) == 0) {
		msg = "This box still has unread messages.";
	}
	else if (strcmp("ER:NEXST", response) == 0) {
		msg = "This box does not exist.";
	}
	else if (strcmp("ER:GREED", response) == 0) {
		msg = "Cannot open multiple boxes at the same time.";
	}
	else if (strcmp("ER:NOOPN", response) == 0) {
		msg = "You currently do not have that message box open.";
	}
	else if (strcmp("ER:EMPTY", response) == 0) {
		msg = "There are no messages left in the box.";
	}
	else if (strcmp("ER:WHAT?", response) == 0) {
		msg = "Your command is in some way broken or malformed.";
	}
	
	if (msg != NULL) printf("%s: %s\n", response, msg);
	else printf("%s\n", response);
}