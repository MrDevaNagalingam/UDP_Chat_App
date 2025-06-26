#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "common.h"
#include "state_machine.h"

typedef struct {
    struct sockaddr_in addr;
    char username[USERNAME_SIZE];
    time_t last_seen;
    int active;
} client_t;

typedef struct {
    int socket;
    server_state_t state;
    int running;
    client_t clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t clients_mutex;
    pthread_t cleanup_tid;
} server_t;

int server_init(server_t *server, int port);
void server_cleanup(server_t *server);
void server_run(server_t *server);
void server_handle_message(server_t *server, char *buffer, struct sockaddr_in *client_addr, socklen_t addr_len);
void server_broadcast_message(server_t *server, char *message, struct sockaddr_in *exclude_addr);
void server_send_to_client(server_t *server, char *message, struct sockaddr_in *client_addr);
int server_add_client(server_t *server, char *username, struct sockaddr_in *addr);
void server_remove_client(server_t *server, struct sockaddr_in *addr);
int server_find_client(server_t *server, struct sockaddr_in *addr);
void *server_cleanup_thread(void *arg);
void server_signal_handler(int sig);

#endif
