
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include "server.h"

box_list_t *init_box_list() {
	box_list_t *list = malloc(sizeof(box_list_t));
	list->front = NULL;
	pthread_mutex_init(&(list->mutex), NULL);

	return list;
}

/*
 * Validates a message for put
 */
int validate_msg(char *mail)
{
    int len = strlen(mail);
    if (len == 0 || mail[0] != '!') return -1;
    mail++;

	char *endptr;
	int num = strtol(mail, &endptr, 10);

    if (errno == EINVAL) return -1;
    if (endptr + num != mail + strlen(mail) - 1) return -1;

    return num;
}

msg_node_t *new_msg(char *msg)
{
    msg_node_t *node = malloc(sizeof(msg_node_t));
    node->msg = malloc(strlen(msg));
	sprintf(node->msg, "%s", msg);
    node->next = NULL;

    return node;
}

msg_queue_t *init_queue()
{
    msg_queue_t *queue = malloc(sizeof(msg_queue_t));
    queue->front = NULL;
    queue->rear = NULL;

    return queue;
}

void enqueue(msg_queue_t *queue, char *msg)
{
    msg_node_t *node = new_msg(msg);

    if (queue->rear == NULL) {
        queue->front = node;
        queue->rear = node;
        return;
    }

    queue->rear->next = node;
    queue->rear = node;
}

/*
 * Dequeues a message.
 * Don't forget to free the message after you receive it.
 * @return Next message in queue, or NULL if queue is empty
 */
char *dequeue(msg_queue_t *queue)
{
    if (queue->front == NULL) {
        return NULL;
    }

    msg_node_t *ptr = queue->front;
	queue->front = queue->front->next;
    char *msg = ptr->msg;
    free(ptr);

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    return msg;
}

box_node_t *new_box(char *name)
{
	box_node_t* box = malloc(sizeof(box_node_t));

	box->name = malloc(strlen(name));
	sprintf(box->name, "%s", name);

	pthread_mutex_init(&(box->mutex), NULL);

	box->msgs = init_queue();
	box->next = NULL;

	return box;
}

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
	if (argc != 2) {
		printf("bruh\n");
		exit(1);
	}

	validate_port(argv[1]);
}

struct sockaddr_in get_addr(char *host, char *port)
{
	struct addrinfo *info;
	struct addrinfo hints = {
		.ai_flags = AI_ADDRCONFIG | AI_PASSIVE,
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

/*
 * Sends a message to another socket.
 */
void send_msg(int sock, char *msg, int len)
{
	if (send(sock, msg, len + 1, 0) < 0) {			// send message to server
			printf("Failed to send\n");
			exit(1);
	}
}

/*
 * Responds to a connecting client.
 */
void hello(int sock)
{
	char *msg = "HELLO DUMBv0 ready!";
	int len = strlen(msg);
	send_msg(sock, msg, len);
}

/*
 * Interprets a client command.
 */
void handle_msg(char* msg, int sock)
{
	int len = strlen(msg);
	if (len < PROTOCOL_LEN) {
		handle_error(sock, "ER:WHAT?");
		return;
	}

	char protocol[PROTOCOL_LEN + 1] = {'\0'};
	memcpy(protocol, msg, 5);

	char *arg;

	if (strcmp(msg, "HELLO") == 0) {
		hello(sock);
	}
	else if (strcmp(protocol, "CREAT") == 0 && len > 5) {
		if (msg[5] == ' ') create_box(sock, msg + 6);
		else {
			handle_error(sock, "ER:WHAT?");
		}
	}
	else if (strcmp(protocol, "DELBX") == 0 && len > 5) {
		if (msg[5] == ' ') delete_box(sock, msg + 6);
		else {
			handle_error(sock, "ER:WHAT?");
		}
	}
	else if (strcmp(protocol, "OPNBX") == 0 && len > 5) {
		if (msg[5] == ' ') open_box(sock, msg + 6);
		else {
			handle_error(sock, "ER:WHAT?");
		}
	}
	else if (strcmp(protocol, "CLSBX") == 0 && len > 5) {
		if (msg[5] == ' ') close_box(sock, msg + 6);
		else {
			handle_error(sock, "ER:WHAT?");
		}
	}
	else if (strcmp(msg, "NXTMG") == 0) {
		next_msg(sock);
	}
	else if (strcmp(protocol, "PUTMG") == 0 && len > 5) {
		if (msg[5] == '!') put_msg(sock, msg + 5);
		else {
			handle_error(sock, "ER:WHAT?");
		}
	}
	else if (strcmp(msg, "GDBYE") == 0) {
		quit(sock);
	}
	else
	{
		handle_error(sock, "ER:WHAT?");
	}
}