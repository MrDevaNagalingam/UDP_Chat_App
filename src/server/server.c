#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server.h"
#include "utils.h"

int server_init(server_t *server, int port) {
    server->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server->socket < 0) {
        perror("Socket creation failed");
        server_transition_to(&server->state, SERVER_STATE_SHUTTING_DOWN);
        return 0;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server->socket);
        server_transition_to(&server->state, SERVER_STATE_SHUTTING_DOWN);
        return 0;
    }
    
    server->state = SERVER_STATE_INITIALIZING;
    server->running = 1;
    server->client_count = 0;
    memset(server->clients, 0, sizeof(server->clients));
    pthread_mutex_init(&server->clients_mutex, NULL);
    return 1;
}

void server_cleanup(server_t *server) {
    server->running = 0;
    pthread_cancel(server->cleanup_tid);
    close(server->socket);
    pthread_mutex_destroy(&server->clients_mutex);
    server_transition_to(&server->state, SERVER_STATE_SHUTTING_DOWN);
}

void server_run(server_t *server) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    server_transition_to(&server->state, SERVER_STATE_RUNNING);
    printf("🚀 UDP Chat Server started on port %d\n", DEFAULT_PORT);
    printf("Waiting for clients...\n");
    printf("Press Ctrl+C to stop the server\n");
    printf("=====================================\n");
    
    pthread_create(&server->cleanup_tid, NULL, server_cleanup_thread, server);
    
    while (server->running) {
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytes_received = recvfrom(server->socket, buffer, BUFFER_SIZE - 1, 0, 
                                        (struct sockaddr*)&client_addr, &addr_len);
        
        if (bytes_received < 0) {
            if (server->running) {
                perror("recvfrom failed");
                server_transition_to(&server->state, SERVER_STATE_SHUTTING_DOWN);
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (strlen(buffer) > 0) {
            server_handle_message(server, buffer, &client_addr, addr_len);
        }
    }
}
