#include <stdio.h>
#include <string.h>
#include "server.h"
#include "utils.h"

void server_handle_message(server_t *server, char *buffer, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char *token, *command, *content;
    char response[BUFFER_SIZE];
    char formatted_msg[BUFFER_SIZE];
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    
    char temp_buffer[BUFFER_SIZE];
    strcpy(temp_buffer, buffer);
    
    token = strtok(temp_buffer, ":");
    if (token == NULL) return;
    
    command = token;
    content = strtok(NULL, "\n");
    
    if (content == NULL) return;
    
    pthread_mutex_lock(&server->clients_mutex);
    
    if (strcmp(command, "JOIN") == 0) {
        if (server_add_client(server, content, client_addr)) {
            snprintf(response, BUFFER_SIZE, "👋 %s joined the chat!", content);
            print_timestamp();
            printf("✅ %s joined from %s:%d\n", content, 
                   inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
            
            snprintf(formatted_msg, BUFFER_SIZE, "Welcome to UDP Chat, %s! 🎉", content);
            server_send_to_client(server, formatted_msg, client_addr);
            
            server_broadcast_message(server, response, client_addr);
        } else {
            snprintf(response, BUFFER_SIZE, "❌ Failed to join chat (server full or error)");
            server_send_to_client(server, response, client_addr);
        }
    } else {
        int client_index = server_find_client(server, client_addr);
        if (client_index >= 0) {
            server->clients[client_index].last_seen = current_time;
            
            snprintf(formatted_msg, BUFFER_SIZE, "[%02d:%02d:%02d] %s: %s",
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                    server->clients[client_index].username, content);
            
            print_timestamp();
            printf("💬 %s\n", formatted_msg);
            
            server_broadcast_message(server, formatted_msg, client_addr);
        } else {
            snprintf(response, BUFFER_SIZE, "❌ Please join first by sending JOIN:YourUsername");
            server_send_to_client(server, response, client_addr);
        }
    }
    
    pthread_mutex_unlock(&server->clients_mutex);
}

void server_broadcast_message(server_t *server, char *message, struct sockaddr_in *exclude_addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active) {
            if (exclude_addr && 
                server->clients[i].addr.sin_addr.s_addr == exclude_addr->sin_addr.s_addr &&
                server->clients[i].addr.sin_port == exclude_addr->sin_port) {
                continue;
            }
            
            server_send_to_client(server, message, &server->clients[i].addr);
        }
    }
}

void server_send_to_client(server_t *server, char *message, struct sockaddr_in *client_addr) {
    ssize_t bytes_sent = sendto(server->socket, message, strlen(message), 0,
                               (struct sockaddr*)client_addr, sizeof(*client_addr));
    if (bytes_sent < 0) {
        perror("sendto failed");
    }
}

int server_add_client(server_t *server, char *username, struct sockaddr_in *addr) {
    int existing = server_find_client(server, addr);
    if (existing >= 0) {
        strncpy(server->clients[existing].username, username, USERNAME_SIZE - 1);
        server->clients[existing].username[USERNAME_SIZE - 1] = '\0';
        server->clients[existing].last_seen = time(NULL);
        return 1;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server->clients[i].active) {
            server->clients[i].addr = *addr;
            strncpy(server->clients[i].username, username, USERNAME_SIZE - 1);
            server->clients[i].username[USERNAME_SIZE - 1] = '\0';
            server->clients[i].last_seen = time(NULL);
            server->clients[i].active = 1;
            server->client_count++;
            return 1;
        }
    }
    
    return 0;
}

void server_remove_client(server_t *server, struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active && 
            server->clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            server->clients[i].addr.sin_port == addr->sin_port) {
            
            char leave_msg[BUFFER_SIZE];
            snprintf(leave_msg, BUFFER_SIZE, "👋 %s left the chat", server->clients[i].username);
            
            print_timestamp();
            printf("❌ %s disconnected from %s:%d\n", server->clients[i].username,
                   inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
            
            server->clients[i].active = 0;
            memset(&server->clients[i], 0, sizeof(client_t));
            server->client_count--;
            
            server_broadcast_message(server, leave_msg, addr);
            break;
        }
    }
}

int server_find_client(server_t *server, struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].active && 
            server->clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            server->clients[i].addr.sin_port == addr->sin_port) {
            return i;
        }
    }
    return -1;
}
