#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "client.h"
#include "utils.h"

int client_init(client_t *client, const char *server_ip, int server_port, const char *username) {
    client->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->socket < 0) {
        perror("Socket creation failed");
        client_transition_to(&client->state, CLIENT_STATE_ERROR);
        return 0;
    }

    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &client->server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address\n");
        close(client->socket);
        client_transition_to(&client->state, CLIENT_STATE_ERROR);
        return 0;
    }

    strncpy(client->username, username, USERNAME_SIZE - 1);
    client->username[USERNAME_SIZE - 1] = '\0';
    client->state = CLIENT_STATE_DISCONNECTED;
    client->running = 1;
    return 1;
}

void client_cleanup(client_t *client) {
    client->running = 0;
    pthread_cancel(client->listen_tid);
    close(client->socket);
}

int client_connect(client_t *client) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "JOIN:%s", client->username);
    client_transition_to(&client->state, CLIENT_STATE_CONNECTING);
    
    if (sendto(client->socket, buffer, strlen(buffer), 0, 
               (struct sockaddr*)&client->server_addr, sizeof(client->server_addr)) < 0) {
        perror("Failed to send JOIN message");
        client_transition_to(&client->state, CLIENT_STATE_ERROR);
        return 0;
    }
    
    client_transition_to(&client->state, CLIENT_STATE_CONNECTED);
    return 1;
}

void *client_listen_thread(void *arg) {
    client_t *client = (client_t *)arg;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client->server_addr);
    
    while (client->running) {
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytes_received = recvfrom(client->socket, buffer, BUFFER_SIZE - 1, 0,
                                        (struct sockaddr*)&client->server_addr, &addr_len);
        
        if (bytes_received < 0) {
            if (client->running) {
                perror("recvfrom failed");
                client_transition_to(&client->state, CLIENT_STATE_ERROR);
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (strlen(buffer) > 0) {
            printf("\r%s\n", buffer);
            printf("[%s] ", client->username);
            fflush(stdout);
        }
    }
    
    return NULL;
}
