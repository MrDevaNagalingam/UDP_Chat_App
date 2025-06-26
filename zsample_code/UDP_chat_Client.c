#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 32
#define DEFAULT_PORT 12345

// Global variables
int client_socket;
struct sockaddr_in server_addr;
char username[USERNAME_SIZE];
int client_running = 1;

// Function prototypes
void *listen_thread(void *arg);
void signal_handler(int sig);
void clear_screen();
void show_help();

int main(int argc, char *argv[]) {
    char server_ip[16] = "127.0.0.1";
    int server_port = DEFAULT_PORT;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    
    // Handle command line arguments
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
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Get username
    printf("🌟 UDP Chat Client\n");
    printf("Server: %s:%d\n", server_ip, server_port);
    printf("─────────────────────────\n");
    printf("Enter your username: ");
    fflush(stdout);
    
    if (fgets(username, USERNAME_SIZE, stdin) == NULL) {
        fprintf(stderr, "Failed to read username\n");
        return 1;
    }
    
    // Remove newline from username
    username[strcspn(username, "\n")] = '\0';
    
    if (strlen(username) == 0) {
        fprintf(stderr, "❌ Username cannot be empty!\n");
        return 1;
    }
    
    // Create UDP socket
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address\n");
        close(client_socket);
        return 1;
    }
    
    printf("🚀 Connecting to chat server...\n");
    
    // Send JOIN message
    snprintf(buffer, BUFFER_SIZE, "JOIN:%s", username);
    if (sendto(client_socket, buffer, strlen(buffer), 0, 
               (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to send JOIN message");
        close(client_socket);
        return 1;
    }
    
    printf("✅ Connected! Type your messages (or type '/help' for commands)\n");
    printf("══════════════════════════════════════════════════════════════\n");
    
    // Start listening thread
    pthread_t listen_tid;
    if (pthread_create(&listen_tid, NULL, listen_thread, NULL) != 0) {
        perror("Failed to create listening thread");
        close(client_socket);
        return 1;
    }
    
    // Main input loop
    while (client_running) {
        printf("[%s] ", username);
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Remove newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        // Handle commands
        if (strcmp(buffer, "/quit") == 0 || strcmp(buffer, "/exit") == 0 || strcmp(buffer, "/q") == 0) {
            printf("👋 Goodbye!\n");
            break;
        } else if (strcmp(buffer, "/help") == 0) {
            show_help();
            continue;
        } else if (strcmp(buffer, "/clear") == 0) {
            clear_screen();
            continue;
        }
        
        // Send regular message
        snprintf(message, BUFFER_SIZE, "%s:%s", username, buffer);
        if (sendto(client_socket, message, strlen(message), 0, 
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Failed to send message");
        }
    }
    
    // Cleanup
    client_running = 0;
    pthread_cancel(listen_tid);
    close(client_socket);
    
    return 0;
}

void *listen_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);
    
    while (client_running) {
        memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytes_received = recvfrom(client_socket, buffer, BUFFER_SIZE - 1, 0,
                                        (struct sockaddr*)&server_addr, &addr_len);
        
        if (bytes_received < 0) {
            if (client_running) {
                perror("recvfrom failed");
            }
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (strlen(buffer) > 0) {
            // Clear current input line and print message
            printf("\r%s\n", buffer);
            printf("[%s] ", username);
            fflush(stdout);
        }
    }
    
    return NULL;
}

void signal_handler(int sig) {
    printf("\n👋 Goodbye!\n");
    client_running = 0;
    close(client_socket);
    exit(0);
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    printf("✅ Connected! Type your messages (or type '/help' for commands)\n");
    printf("══════════════════════════════════════════════════════════════\n");
}

void show_help() {
    printf("\n📖 UDP Chat Commands:\n");
    printf("  /help  - Show this help message\n");
    printf("  /clear - Clear the screen\n");
    printf("  /quit  - Exit the chat (/exit or /q also work)\n");
    printf("  Type any message to send it to everyone!\n");
    printf("──────────────────────────────────────────────────────────────\n");
}
