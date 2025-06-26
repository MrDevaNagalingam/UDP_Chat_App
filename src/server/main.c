#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"
#include "utils.h"

server_t *global_server = NULL;

void server_signal_handler(int sig) {
    if (global_server) {
        printf("\n🛑 Shutting down server...\n");
        server_cleanup(global_server);
    }
    printf("🛑 Server stopped\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    server_t server;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number\n");
            return 1;
        }
    }
    
    signal(SIGINT, server_signal_handler);
    signal(SIGTERM, server_signal_handler);
    
    if (!server_init(&server, port)) {
        return 1;
    }
    
    global_server = &server;
    server_run(&server);
    server_cleanup(&server);
    
    return 0;
}
