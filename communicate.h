#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define TOTAL_PROCESSES 4
#define BUFFER_SIZE 256

// struct for the accept thread argument
struct s_accept {
    int MACHINE_ID;
    int* network_sockets;
    int* open_sockets;
};

// struct for the listen thread argument
struct s_listen {
    int network_socket;
    int* open_sockets;
    int ID;
};

void send_message(int network_socket, char message[BUFFER_SIZE]);
void* accepting_input(void *data);
char* parse_accepting_input(char* input, int* status_code);
void* listen_messages(void* data);
void exit_process(int* open_sockets, int* network_sockets);
void received_stop_message(int* open_sockets, int ID, int* status_code);

#endif