#include <stdio.h>
#include <unistd.h>
#include "server.h"

void *server_cleanup_thread(void *arg) {
    server_t *server = (server_t *)arg;
    while (server->running) {
        sleep(60);
        
        pthread_mutex_lock(&server->clients_mutex);
        time_t current_time = time(NULL);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].active && 
                (current_time - server->clients[i].last_seen) > TIMEOUT_SECONDS) {
                server_remove_client(server, &server->clients[i].addr);
            }
        }
        
        pthread_mutex_unlock(&server->clients_mutex);
    }
    return NULL;
}
