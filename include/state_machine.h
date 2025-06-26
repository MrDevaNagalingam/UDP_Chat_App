#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

typedef enum {
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED,
    CLIENT_STATE_ERROR
} client_state_t;

typedef enum {
    SERVER_STATE_INITIALIZING,
    SERVER_STATE_RUNNING,
    SERVER_STATE_SHUTTING_DOWN
} server_state_t;

void client_transition_to(client_state_t *current_state, client_state_t new_state);
void server_transition_to(server_state_t *current_state, server_state_t new_state);
const char *client_state_to_string(client_state_t state);
const char *server_state_to_string(server_state_t state);

#endif
