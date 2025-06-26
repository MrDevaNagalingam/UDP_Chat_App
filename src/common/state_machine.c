#include <stdio.h>
#include "state_machine.h"

void client_transition_to(client_state_t *current_state, client_state_t new_state) {
    printf("Client state transition: %s -> %s\n", 
           client_state_to_string(*current_state), 
           client_state_to_string(new_state));
    *current_state = new_state;
}

void server_transition_to(server_state_t *current_state, server_state_t new_state) {
    printf("Server state transition: %s -> %s\n", 
           server_state_to_string(*current_state), 
           server_state_to_string(new_state));
    *current_state = new_state;
}

const char *client_state_to_string(client_state_t state) {
    switch (state) {
        case CLIENT_STATE_DISCONNECTED: return "DISCONNECTED";
        case CLIENT_STATE_CONNECTING: return "CONNECTING";
        case CLIENT_STATE_CONNECTED: return "CONNECTED";
        case CLIENT_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char *server_state_to_string(server_state_t state) {
    switch (state) {
        case SERVER_STATE_INITIALIZING: return "INITIALIZING";
        case SERVER_STATE_RUNNING: return "RUNNING";
        case SERVER_STATE_SHUTTING_DOWN: return "SHUTTING_DOWN";
        default: return "UNKNOWN";
    }
}
