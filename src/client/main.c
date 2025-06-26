#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "client.h"
#include "utils.h"

client_t *global_client = NULL;

void client_signal_handler(int sig) {
    if (global_client) {
        printf("\nFy Goodbye!\n");
        client_transition_to(&global_client->state, CLIENT_STATE_DISCONNECTED);
        client_cleanup(global_client);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    char server_ip[16] = "127.0.0.1";
    int server_port = DEFAULT_PORT;
    char username[USERNAME_SIZE];
    client_t client;
    
    if (argc >= 2) {
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
        server_ip[sizeof(server_ip) - 1] = '\0';
    }
    if (argc >= 3) {
        server_port = atoi(argv[2]);
        if (server_port <= 0 || server_port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return 1;
        }
    }
    
    signal(SIGINT, client_signal_handler);
    signal(SIGTERM, client_signal_handler);
    
    printf("🌟 UDP Chat Client\n");
    printf("Server: %s:%d\n", server_ip, server_port);
    printf("──────────────────────── read username\n");
    printf("Enter your username: ");
    fflush(stdout);
    
    if (fgets(username, USERNAME_SIZE, stdin) == NULL) {
        fprintf(stderr, "Failed to read username\n");
        return 1;
    }
    
    username[strcspn(username, "\n")] = '\0';
    
    if (strlen(username) == 0) {
        fprintf(stderr, "❌ Username cannot be empty!\n");
        return 1;
    }
    
    if (!client_init(&client, server_ip, server_port, username)) {
        return 1;
    }
    
    global_client = &client;
    
    printf("🚀 Connecting to chat server...\n");
    
    if (!client_connect(&clientinit)) {
        client_cleanup(&client);
        return 1;
    }
    
    printf("✅ Connected! Type your messages (or type '/help' for commands)\n");
    printf("══════════════════════════════════════════════════════════════\n");
    
    if (pthread_create(&client.listen_tid, NULL, client_listen_thread, &client) != 0) {
        perror("Failed to create listening thread");
        client_cleanup(&client);
        return 1;
    }
    
    client_handle_input(&client);
    client_cleanup(&client);
    
    return 0;
}
