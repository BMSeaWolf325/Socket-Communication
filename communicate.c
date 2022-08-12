#include "communicate.h"

void send_message(int network_socket, char message[]) {
    // send message to sockfd network_socket
    if (send(network_socket, message, sizeof(char) * BUFFER_SIZE, 0) == -1)
    {
        fprintf(stderr,"Socket Send Failed.\n");
        exit(1);
    }
}

void* accepting_input(void* data)
{
    struct s_accept *params = data;
    int MACHINE_ID = params->MACHINE_ID;
    int* network_sockets = params->network_sockets;
    int* open_sockets = params->open_sockets;
    
    while(1)
    {
        char user_input[BUFFER_SIZE];
        memset(user_input, '\0', BUFFER_SIZE);
        char *message;
        int parse_output;

        // get user input
        fgets(user_input, BUFFER_SIZE, stdin);
        user_input[strlen(user_input) - 1] = '\0';

        // parse the user input
        message = parse_accepting_input(user_input, &parse_output);
        
        if (parse_output == MACHINE_ID)
        {
            fprintf(stderr, "Invalid Input. Can not send a message to the current machine.\n");
            free(message);
        }
        else if (parse_output == 0)
        {
            // send message to all open sockets (processes)
            int i;
            for (i = 0; i < TOTAL_PROCESSES; i++)
            {
                if (open_sockets[i] == 1)
                {
                    send_message(network_sockets[i], message);
                }
            }
            free(message);
        }
        else if (parse_output > 0)
        {
            // send a message to a particular socket (process)
            if (open_sockets[parse_output - 1] == 1)
            {
                send_message(network_sockets[parse_output - 1], message);
            }
            else
            {
                printf("Socket is Closed Error.\n");
            }
            free(message);
        }
        else if (parse_output == -1)
        {
            // stop message
            // performs the exit_process function then terminates itself
            exit_process(open_sockets, network_sockets);
            printf("Sent Stop Message to all Open Sockets. Exiting this Process.\n");
            free(message);
            exit(1);
        }
    }
    free(params);
}

void exit_process(int* open_sockets, int* network_sockets)
{
    int i;
    
    // send a unique message to all open processs to tell them that this process is stopped
    for (i = 0; i < TOTAL_PROCESSES; i++)
    {
        if (open_sockets[i] == 1)
        {
            char message[BUFFER_SIZE] = "8p9AJDGNNFhp1SW3zfohyOB1kDkNjKqtPBrKR1Fdqlio0yVEAZ";
            send_message(network_sockets[i], message);
            if (close(network_sockets[i]) == -1)
            {
                fprintf(stderr,"Socket Close Failed.\n");
                exit(1);
            }
            open_sockets[i] = 0;
        }
    }
}

char* parse_accepting_input(char* input, int* status_code) {
    // parses the user input
    // sets the status code respectively
    // returns the message that the user inputted if its in the valid format
    char* token = strtok(input, " ");
    if (strcmp("Stop", token) == 0)
    {
        *status_code = -1;
        return NULL;
    }
    else if (strcmp("send", token) == 0)
    {
        char num[BUFFER_SIZE];
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            strncpy(num, token, sizeof(token));
            if (atoi(num) >= 0 && atoi(num) <= TOTAL_PROCESSES)
            {
                token = strtok(NULL, " ");
                if (token != NULL && (strtok(NULL, " ") == NULL))
                {
                    *status_code = atoi(num);
                    return strdup(token);
                }
            }
        }
    }
    fprintf(stderr,"Invalid Input.\n");
    *status_code = -2;
    return NULL;
}

void* listen_messages(void* data)
{  
    // listens for messages at a particular network_socket (sockfd)
    struct s_listen *params = data;
    int ID = params->ID;
    int network_socket = params->network_socket;
    int* open_sockets = params->open_sockets;

    while(1)
    {
        // receives the message
        char server_response[BUFFER_SIZE] = "";
        size_t rcv_status = 0;
        while (rcv_status == 0)
        {
            memset(server_response, '\0', BUFFER_SIZE);
            rcv_status = recv(network_socket, &server_response, sizeof(char) * BUFFER_SIZE, 0);
        }
        if (rcv_status == -1)
        {
            fprintf(stderr,"Socket Receive Failed.\n");
            exit(1);
        }
        // if the message is the stop message, then call the received_stop_message function
        if (strcmp("8p9AJDGNNFhp1SW3zfohyOB1kDkNjKqtPBrKR1Fdqlio0yVEAZ", server_response) == 0)
        {
            int status_code;
            received_stop_message(open_sockets, ID, &status_code);
            // if there are no open socket(s) remaining, exit the process
            if (status_code == 0)
            {
                printf("All Sockets are Closed. Exiting this Process.\n");
                exit(1);
            }
        }
        // otherwise print the message received and from which process it came from
        else
        {
            printf("Process %d sent: %s\n", ID + 1, server_response);
        }
    }
    free(params);
}

void received_stop_message(int* open_sockets, int ID, int* status_code)
{
    // set the socket to closed
    if (open_sockets[ID] == 1)
    {
        open_sockets[ID] = 0;
    }
    int flag = 0, i = 0;

    // check if any sockets are open
    for (; i < TOTAL_PROCESSES; i++)
    {
        if (open_sockets[i] == 1)
        {
            flag = 1;
        }
    }

    // if there is still open socket(s)
    if (flag == 1)
    {
        *status_code = 1;
    }
    // if there is no open sockets remaining
    else
    {
        *status_code = 0;
    }
}