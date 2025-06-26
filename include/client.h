#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <pthread.h>
#include "common.h"
#include "state_machine.h"

typedef struct {
    int socket;
    struct sockaddr_in server_addr;
    char username[USERNAME_SIZE];
    client_state_t state;
    int running;
    pthread_t listen_tid;
} client_t;

int client_init(client_t *client, const char *server_ip, int server_port, const char *username);
void client_cleanup(client_t *client);
int client_connect(client_t *client);
void client_handle_input(client_t *client);
void *client_listen_thread(void *arg);
void client_signal_handler(int sig);
void client_show_help();

#endif
