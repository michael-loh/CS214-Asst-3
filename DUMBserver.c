
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "server.h"

box_list_t *box_list = NULL;

__thread box_node_t *selected_box;
__thread char ip[INET_ADDRSTRLEN];

/*
 * Delivers an error
 */
void handle_error(int sock, char *err)
{
	time_t event_time;
	struct tm *info;
	char timebuf[80];
	time(&event_time);
	info = localtime(&event_time);
	strftime(timebuf, 80, "%H%M %d %b", info);

	send_msg(sock, err, strlen(err));
	fprintf(stderr, "%s %s %s\n", timebuf, ip, err);
}

/*
 * Prints the names of all the boxes
 */
void print_boxes()
{
	box_node_t* ptr = box_list->front;
	printf("boxes: ");
	while(ptr != NULL) {
		printf("%s -> ", ptr->name);
		ptr = ptr->next;
	}
	printf("NULL\n");
}

/*
 * Creates a box
 */
void create_box(int sock, char* box_name)
{
	char *msg;

	box_node_t *ptr = box_list->front;
	pthread_mutex_lock(&(box_list->mutex));

	while(ptr != NULL) {							// iterate to end of list
		if (strcmp(ptr->name, box_name) == 0) {		// if already exists, error
			handle_error(sock, "ER:EXIST");
			pthread_mutex_unlock(&(box_list->mutex));
			return;
		}
		ptr = ptr->next;
	}

	box_node_t* box = new_box(box_name);			// create the box with name provided by client
	box->next = box_list->front;					// add box to front of list
	box_list->front = box;							// set front to new box

	pthread_mutex_unlock(&(box_list->mutex));

	msg = "OK!";
	send_msg(sock, msg, strlen(msg));

	// print_boxes();
}

/*
 * Deletes a box
 */
void delete_box(int sock, char* box_name)
{
	char *msg;

	box_node_t *ptr = box_list->front;
	box_node_t *prev = NULL;

	pthread_mutex_lock(&(box_list->mutex));	// lock list

	while(ptr != NULL) {
		if (strcmp(box_name, ptr->name) == 0) {
			pthread_mutex_unlock(&(box_list->mutex));	// unlock list before locking box
			if (pthread_mutex_trylock(&(ptr->mutex)) != 0) {
				handle_error(sock, "ER:OPEND");
				return;
			}

			msg_node_t *peek = ptr->msgs->front;	// check if box is empty
			if (peek != NULL) {
				pthread_mutex_unlock(&(ptr->mutex));
				handle_error(sock, "ER:NOTMT");
				return;
			}

			if (prev == NULL) {
				box_list->front = ptr->next;
			} else {
				prev->next = ptr->next;
			}

			// print_boxes();

			pthread_mutex_unlock(&(ptr->mutex));
			pthread_mutex_destroy(&(ptr->mutex));		// destroy mutex
			free(ptr->msgs);						// free queue
			free(ptr);								// free box

			msg = "OK!";
			send_msg(sock, msg, strlen(msg));

			return;
		}

		prev = ptr;
		ptr = ptr->next;
	}

	pthread_mutex_unlock(&(box_list->mutex));

	handle_error(sock, "ER:NEXST");

	return;
}

/*
 * Lets client open a box
 */
void open_box(int sock, char* box_name)
{
	char* msg;

	box_node_t* ptr = box_list->front;

	pthread_mutex_lock(&(box_list->mutex));			// lock list

	if (selected_box != NULL) {						// cannot open multiple boxes
		handle_error(sock, "ER:GREED");
		pthread_mutex_unlock(&(box_list->mutex));
		return;
	}

	while(ptr != NULL) {							// look for box
		if (strcmp(box_name, ptr->name) == 0 ) {	// box found
			pthread_mutex_unlock(&(box_list->mutex));	// unlock list before locking box
			if (pthread_mutex_trylock(&(ptr->mutex)) != 0) {	// check if box is in use
				handle_error(sock, "ER:OPEND");
				return;
			}

			selected_box = ptr;						// update selected box

			msg = "OK!";
			send_msg(sock, msg, strlen(msg));		// report success
			return;
		}
		ptr = ptr->next;
	}

	pthread_mutex_unlock(&(box_list->mutex));

	handle_error(sock, "ER:NEXST");
	return;
}

/*
 * Closes the box a client has open
 */
void close_box(int sock, char *name)
{
	char *msg;
	time_t event_time;
	struct tm *info;
	char timebuf[80];

	if (selected_box == NULL || strcmp(selected_box->name, name) != 0) {	// you have not selected the box
		handle_error(sock, "ER:NOOPN");
		return;
	}

	pthread_mutex_unlock(&(selected_box->mutex));
	selected_box = NULL;
	msg = "OK!";
	send_msg(sock, msg, strlen(msg));
}

/*
 * Retrieves the next message in a box
 */
void next_msg(int sock)
{
	char *msg;
	time_t event_time;
	struct tm *info;
	char timebuf[80];

	if (selected_box == NULL) {				// make sure a box is open
		handle_error(sock, "ER:NOOPN");
		return;
	}

	char *mail;
	mail = dequeue(selected_box->msgs);		// dequeue next message

	if (mail == NULL) {
		handle_error(sock, "ER:EMPTY");
		return;
	}

	asprintf(&msg, "OK%s", mail);			// format it to send to client
	free(mail);
	send_msg(sock, msg, strlen(msg));
	free(msg);

//	print_queue(selected_box->msgs);
}

/*
 * Puts a message in the currently open box
 */
void put_msg(int sock, char *mail)
{
	char *msg;
	time_t event_time;
	struct tm *info;
	char timebuf[80];

	if (selected_box == NULL) {
		handle_error(sock, "ER:NOOPN");
		return;
	}

	int len = validate_msg(mail);
	if (len < 0) {
		handle_error(sock, "ER:WHAT?");
		return;
	}

	enqueue(selected_box->msgs, mail);

	asprintf(&msg, "OK!%d", len);
	send_msg(sock, msg, strlen(msg));
	free(msg);

	// print_queue(selected_box->msgs);
}

/*
 * Responds to a disconnecting client
 */
void quit(int sock)
{
	time_t event_time;
	struct tm *info;
	char timebuf[80];
	time(&event_time);
	info = localtime(&event_time);
	strftime(timebuf, 80, "%H%M %d %b", info);

	if (selected_box != NULL) {
		pthread_mutex_unlock(&(selected_box->mutex));
		selected_box = NULL;
	}

	if (close(sock) < 0) {
		printf("Failure at close()\n");
	}

	printf("%s %s disconnected\n", timebuf, ip);

	pthread_exit(0);
}

/*
 * Creates a thread to accept an incoming connecting client
 */
void *connect_client(void *param)
{
	thread_param_t* parameters = (thread_param_t*) param;
	int sock = parameters->client_sock;
	inet_ntop( AF_INET, &parameters->client_addr , ip, INET_ADDRSTRLEN );

	time_t event_time;
	struct tm *info;
	char timebuf[80];
	time(&event_time);
	info = localtime(&event_time);
	strftime(timebuf, 80, "%H%M %d %b", info);

	printf("%s %s connected\n", timebuf, ip);

	selected_box = NULL;

	while (1) {
		//receive message from client
		char client_message[MAX_MSG_LEN];

		int err = recv(sock, client_message, MAX_MSG_LEN - 1, 0);

		time(&event_time);
		info = localtime(&event_time);
		strftime(timebuf, 80, "%H%M %d %b", info);

		struct sockkaddr_in* pv4_addr = (struct sockaddr_in*) &sock;

		if (err < 0) {
			printf("Failed to receive\n");
			return NULL;
		} else if (err == 0) {
			quit(sock);
			return NULL;
		}

		printf("%s %s %s\n", timebuf, ip, client_message);

		handle_msg(client_message, sock);

		memset(client_message, '\0', MAX_MSG_LEN);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	validate(argc, argv);

	char *host = NULL;
	char *port = argv[1];

	struct sockaddr_in addr = get_addr(host, port);

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		printf("Failure at socket()\n");
		exit(1);
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
		printf("Failure at setsockopt()\n");
		exit(1);
	}

	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Failure at bind()\n");
		exit(1);
	}

	if (listen(sock, 1024) < 0) {
		printf("Failure at listen()\n");
		exit(1);
	}

	box_list = init_box_list();

	pthread_t thread;
	int client_sock;
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	while (1) {
		int client_sock = accept(sock, (struct sockaddr*) &client_addr, &addr_len);

		thread_param_t* parameter = (thread_param_t*) malloc(sizeof(thread_param_t));
		parameter->client_addr = client_addr;
		parameter->client_sock = client_sock;

		pthread_create(&thread, NULL, connect_client, (void *) parameter);

		pthread_detach(thread);
	}

	return 0;
}
