
#ifndef _SERVER_H
#define _SERVER_H

#define MAX_MSG_LEN 4096
#define PROTOCOL_LEN 5

typedef struct msg_node_t {
    char *msg;
    struct msg_node_t *next;
} msg_node_t;

typedef struct msg_queue_t {
	struct msg_node_t *front;
    struct msg_node_t *rear;
} msg_queue_t;

typedef struct box_node_t {
	char *name;
	pthread_mutex_t mutex;
	msg_queue_t *msgs;
	struct box_node_t* next;
} box_node_t;

typedef struct box_list_t {
	box_node_t *front;
	pthread_mutex_t mutex;
} box_list_t;

typedef struct thread_param_t {
	struct sockaddr_in client_addr;
	int client_sock;
} thread_param_t;

/*
 * Initializes and returns the box list.
 */
box_list_t *init_box_list();

/*
 * Creates a new box with the specified name.
 */
box_node_t *new_box(char *name);

/*
 * Enqueues a message.
 */
void enqueue(msg_queue_t *queue, char *msg);

/*
 * Dequeues a message.
 * @return Next message in queue, or NULL if queue is empty
 */
char *dequeue(msg_queue_t *queue);

/*
 * Validates command line arguments for server.
 */
void validate(int argc, char *argv[]);

/*
 * Retrieves socket address struct for server.
 */
struct sockaddr_in get_addr(char *host, char *port);

/*
 * Sends a message to another socket.
 */
void send_msg(int sock, char *msg, int len);

/*
 * Responds to a connecting client.
 */
void hello(int sock);

/*
 * Validates a message for put
 */
int validate_msg(char *mail);

/*
 * Delivers an error
 */
void handle_error(int sock, char *err);

/*
 * Interprets a client command.
 */
void handle_msg(char* msg, int sock);

#endif
