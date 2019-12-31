
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include "client.h"

/*
 * TODO: recv a response - if it is incorrect, make this eat shit
 */
void hello(int sock)
{
	char *msg = "HELLO";
	send_msg(sock, msg);

	char *expected_response = "HELLO DUMBv0 ready!";
	char response[MAX_MSG_LEN] = {'\0'};

	if (recv(sock, response, MAX_MSG_LEN - 1, 0) < 0) {
		printf("Failed to receive\n");
		exit(1);
	}

	printf("%s\n", response);

	if (strcmp(expected_response, response) != 0) {
		printf("Wrong answer! Terminating program...\n");
		exit(1);
	}
}

int valid_name(char *name)
{
	int len = strlen(name);
	if (len < 5 || len > 25) return 0;
	if ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z')) return 1;
	return 0;
}

/*
 * All of these functions edit msg and len to contain a coherent command
 */
void create_box(char **msg)
{
	printf("> create\nName your new message box:\n");

	char *name;
	read_input(&name);

	while (!valid_name(name)) {
		printf("Please enter a box name between 5-25 characters that begins with an alphabetic character.\n");
		read_input(&name);
	}

	printf("create:> %s\n", name);
	asprintf(msg, "CREAT %s", name);
}

void delete_box(char **msg)
{
	printf("> delete\nPlease specify which message box you would like to delete:\n");

	char *name;
	read_input(&name);

	printf("delete:> %s\n", name);
	asprintf(msg, "DELBX %s", name);
}

void open_box(char **msg)
{
	printf("> open\nPlease specify which message box you would like to open:\n");

	char *name;
	read_input(&name);

	printf("open:> %s\n", name);
	asprintf(msg, "OPNBX %s", name);
}

void close_box(char **msg)
{
	printf("> close\nClose which box?\n");

	char *input;
	read_input(&input);

	printf("close:> %s\n", input);
	asprintf(msg, "CLSBX %s", input);
}

void next_msg(char **msg)
{
	printf("> next\n");
	*msg = "NXTMG";
}

void put_msg(char **msg)
{
	printf("> put\nEnter your message:\n");

	char *input;
	read_input(&input);

	printf("put:> %s\n", input);
	asprintf(msg, "PUTMG!%d!%s", strlen(input), input);
}

void quit(int sock, char **msg)
{
	*msg = "GDBYE";

	send_msg(sock, *msg);

	char response[MAX_MSG_LEN] = {'\0'};

	if (recv(sock, response, MAX_MSG_LEN - 1, 0) == 0) {
		printf("Connection successfully shut down. Good night!\n");
		exit(1);
	}

	printf("Something went wrong. Server response: %s\n", response);
}

void help() 
{
	printf("Commands: create, delete, open, close, next, put, quit\n");
}

/*
 * This method translates an English command
 * into the DUMB protocol command.
 * @return 1 to send message to server, 0 if client handled it
 */
int form_msg(int sock, char **msg) 
{
	if(strcmp("create", *msg) == 0) {
		create_box(msg);
	}
	else if(strcmp("delete", *msg) == 0) {
		delete_box(msg);
	}
	else if(strcmp("open", *msg) == 0) {
		open_box(msg);
	}
	else if(strcmp("close", *msg) == 0) {
		close_box(msg);
	}
	else if(strcmp("next", *msg) == 0) {
		next_msg(msg);
	}
	else if(strcmp("put", *msg) == 0) {
		put_msg(msg);
	}
	else if(strcmp("quit", *msg) == 0) {
		quit(sock, msg);
		return 0;
	}
	else if (strcmp("help", *msg) == 0) {
		help();
		return 0;
	}
	else {
		printf("Unknown command. Please try again.\n");
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	validate(argc, argv);

	char *host = argv[1];
	char *port = argv[2];

	struct sockaddr_in addr = get_addr(host, port);

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		printf("Failure at socket()\n");
		exit(1);
	}

	int i;
	for (i = 0; i < 3; i++) {
		if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == 0) break;
	}
	if (i == 3) {
		printf("Failure at connect()\n");
		exit(1);
	}

	hello(sock);

	while (1) {
		char *msg;

		read_input(&msg);							// reads from stdin
		int has_msg = form_msg(sock, &msg);		// parses and forms input into a message
		
		if (has_msg) {							// if request was handled client side, don't send and recv
			send_msg(sock, msg);

			char response[MAX_MSG_LEN] = {'\0'};

			if (recv(sock, response, MAX_MSG_LEN - 1, 0) < 0) {
				printf("Failed to receive\n");
				exit(1);
			}

			interpret_response(response);
		}
		
	}

	return 0;
}
