#include <stdio.h>
#include <string.h>
#include "client.h"

void client_handle_input(client_t *client) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    
    while (client->running) {
        printf("[%s] ", client->username);
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        if (strcmp(buffer, "/quit") == 0 || strcmp(buffer, "/exit") == 0 || strcmp(buffer, "/q") == 0) {
            printf("👋 Goodbye!\n");
            client_transition_to(&client->state, CLIENT_STATE_DISCONNECTED);
            break;
        } else if (strcmp(buffer, "/help") == 0) {
            client_show_help();
            continue;
        } else if (strcmp(buffer, "/clear") == 0) {
            clear_screen();
            printf("✅ Connected! Type your messages (or type '/help' for commands)\n");
            printf("══════════════════════════════════════════════════════════════\n");
            continue;
        }
        
        snprintf(message, BUFFER_SIZE, "%s:%s", client->username, buffer);
        if (sendto(client->socket, message, strlen(message), 0, 
                   (struct sockaddr*)&client->server_addr, sizeof(client->server_addr)) < 0) {
            perror("Failed to send message");
            client_transition_to(&client->state, CLIENT_STATE_ERROR);
        }
    }
}

void client_show_help() {
    printf("\n📖 UDP Chat Commands:\n");
    printf("  /help  - Show this help message\n");
    printf("  /clear - Clear the screen\n");
    printf("  /quit  - Exit the chat (/exit or /q also work)\n");
    printf("  Type any message to send it to everyone!\n");
    printf("──────────────────────────────────────────────────────────────\n");
}
