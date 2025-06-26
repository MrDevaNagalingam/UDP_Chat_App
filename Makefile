CC = gcc
CFLAGS = -Wall -pthread
INCLUDE = -Iinclude
SRC_DIR = src
CLIENT_SRC = $(SRC_DIR)/client/main.c $(SRC_DIR)/client/client.c $(SRC_DIR)/client/input.c $(SRC_DIR)/common/state_machine.c $(SRC_DIR)/common/utils.c
SERVER_SRC = $(SRC_DIR)/server/main.c $(SRC_DIR)/server/server.c $(SRC_DIR)/server/clients.c $(SRC_DIR)/server/network.c $(SRC_DIR)/common/state_machine.c $(SRC_DIR)/common/utils.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_EXEC = udp_chat_client
SERVER_EXEC = udp_chat_server

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_EXEC) $(SERVER_EXEC)

.PHONY: all clean
