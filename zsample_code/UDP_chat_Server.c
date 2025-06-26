#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 32
#define DEFAULT_PORT 12345

// Client structure to store client information
typedef struct {
    struct sockaddr_in addr;
    char username[USERNAME_SIZE];
    time_t last_seen;
    int active;
} client_t;

// Global variables
client_t clients[MAX_CLIENTS];
int client_count = 0;
int server_socket;
int server_running = 1;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Function prototypes
void *cleanup_thread(void *arg);
void handle_message(char *buffer, struct sockaddr_in *client_addr, socklen_t addr_len);
void broadcast_message(char *message, struct sockaddr_in *exclude_addr);
void send_to_client(char *message, struct sockaddr_in *client_addr);
int add_client(char *username, struct sockaddr_in *addr);
void remove_client(struct sockaddr_in *addr);
int find_client(struct sockaddr_in *addr);
void signal_handler(int sig);
void print_timestamp();

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int port = DEFAULT_PORT;
    
    // Handle command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return 1;
        }
    }
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize clients array
    memset(clients, 0, sizeof(clients));
    
    // Create UDP socket
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket to address
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }
    
    printf("🚀 UDP Chat Server started on port %d\n", port);
    printf("Waiting for clients...\n");
    printf("Press Ctrl+C to stop the server\n");
    printf("=====================================\n");
    
    // Start cleanup thread
    pthread_t cleanup_tid;
    pthread_create(&cleanup_tid, NULL, cleanup_thread, NULL);
    
    // Main server loop
    while (server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0, 
                                        (struct sockaddr*)&client_addr, &addr_len);
        
        if (bytes_received < 0) {
            if (server_running) {
                perror("recvfrom failed");
            }
            break;
        }
        
        buffer[bytes_received] = '\0';  // Null terminate
        
        if (strlen(buffer) > 0) {
            handle_message(buffer, &client_addr, addr_len);
        }
    }
    
    // Cleanup
    pthread_cancel(cleanup_tid);
    close(server_socket);
    pthread_mutex_destroy(&clients_mutex);
    printf("\n🛑 Server stopped\n");
    
    return 0;
}

void handle_message(char *buffer, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char *token;
    char *command;
    char *content;
    char response[BUFFER_SIZE];
    char formatted_msg[BUFFER_SIZE];
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    
    // Parse message format: "COMMAND:CONTENT"
    char temp_buffer[BUFFER_SIZE];
    strcpy(temp_buffer, buffer);
    
    token = strtok(temp_buffer, ":");
    if (token == NULL) return;
    
    command = token;
    content = strtok(NULL, "\n");  // Get rest of message
    
    if (content == NULL) return;
    
    pthread_mutex_lock(&clients_mutex);
    
    if (strcmp(command, "JOIN") == 0) {
        // New client joining
        if (add_client(content, client_addr)) {
            snprintf(response, BUFFER_SIZE, "👋 %s joined the chat!", content);
            print_timestamp();
            printf("✅ %s joined from %s:%d\n", content, 
                   inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
            
            // Send welcome message to new client
            snprintf(formatted_msg, BUFFER_SIZE, "Welcome to UDP Chat, %s! 🎉", content);
            send_to_client(formatted_msg, client_addr);
            
            // Broadcast join message to others
            broadcast_message(response, client_addr);
        } else {
            snprintf(response, BUFFER_SIZE, "❌ Failed to join chat (server full or error)");
            send_to_client(response, client_addr);
        }
    } else {
        // Regular message from existing client
        int client_index = find_client(client_addr);
        if (client_index >= 0) {
            // Update last seen time
            clients[client_index].last_seen = current_time;
            
            // Format message with timestamp and username
            snprintf(formatted_msg, BUFFER_SIZE, "[%02d:%02d:%02d] %s: %s",
                    tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                    clients[client_index].username, content);
            
            print_timestamp();
            printf("💬 %s\n", formatted_msg);
            
            // Broadcast to all other clients
            broadcast_message(formatted_msg, client_addr);
        } else {
            snprintf(response, BUFFER_SIZE, "❌ Please join first by sending JOIN:YourUsername");
            send_to_client(response, client_addr);
        }
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(char *message, struct sockaddr_in *exclude_addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            // Skip the sender
            if (exclude_addr && 
                clients[i].addr.sin_addr.s_addr == exclude_addr->sin_addr.s_addr &&
                clients[i].addr.sin_port == exclude_addr->sin_port) {
                continue;
            }
            
            send_to_client(message, &clients[i].addr);
        }
    }
}

void send_to_client(char *message, struct sockaddr_in *client_addr) {
    ssize_t bytes_sent = sendto(server_socket, message, strlen(message), 0,
                               (struct sockaddr*)client_addr, sizeof(*client_addr));
    if (bytes_sent < 0) {
        perror("sendto failed");
    }
}

int add_client(char *username, struct sockaddr_in *addr) {
    // Check if client already exists
    int existing = find_client(addr);
    if (existing >= 0) {
        // Update existing client
        strncpy(clients[existing].username, username, USERNAME_SIZE - 1);
        clients[existing].username[USERNAME_SIZE - 1] = '\0';
        clients[existing].last_seen = time(NULL);
        return 1;
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].addr = *addr;
            strncpy(clients[i].username, username, USERNAME_SIZE - 1);
            clients[i].username[USERNAME_SIZE - 1] = '\0';
            clients[i].last_seen = time(NULL);
            clients[i].active = 1;
            client_count++;
            return 1;
        }
    }
    
    return 0;  // Server full
}

void remove_client(struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) {
            
            char leave_msg[BUFFER_SIZE];
            snprintf(leave_msg, BUFFER_SIZE, "👋 %s left the chat", clients[i].username);
            
            print_timestamp();
            printf("❌ %s disconnected from %s:%d\n", clients[i].username,
                   inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
            
            clients[i].active = 0;
            memset(&clients[i], 0, sizeof(client_t));
            client_count--;
            
            broadcast_message(leave_msg, addr);
            break;
        }
    }
}

int find_client(struct sockaddr_in *addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && 
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) {
            return i;
        }
    }
    return -1;
}

void *cleanup_thread(void *arg) {
    while (server_running) {
        sleep(60);  // Check every minute
        
        pthread_mutex_lock(&clients_mutex);
        time_t current_time = time(NULL);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && 
                (current_time - clients[i].last_seen) > 300) {  // 5 minutes timeout
                remove_client(&clients[i].addr);
            }
        }
        
        pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

void signal_handler(int sig) {
    printf("\n🛑 Shutting down server...\n");
    server_running = 0;
    close(server_socket);
}

void print_timestamp() {
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    printf("[%02d:%02d:%02d] ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}
