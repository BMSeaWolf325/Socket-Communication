#include <pthread.h>
#include <errno.h>
#include "communicate.h"

int main(int argc, char *argv[])
{   
    if (argc < 2)
    {
        fprintf(stderr,"Must Enter Machine ID.\n");
        exit(1);
    }

    const int MACHINE_ID = atoi(argv[1]);
    if (MACHINE_ID < 1 || MACHINE_ID > TOTAL_PROCESSES)
    {
        fprintf(stderr,"Invalid Machine ID.\n");
        exit(1);
    }

    int network_sockets[TOTAL_PROCESSES]; // int array of the socket sockfds
    int open_sockets[TOTAL_PROCESSES]; // "boolean" array to keep track of the open sockets
    int i;

    // intitialize each socket to false
    for (i = 0; i < TOTAL_PROCESSES; i++)
    {
        open_sockets[i] = 0;
    }

    // IPS of machines 1-4
    char *IPS[TOTAL_PROCESSES];
    IPS[0] = "10.176.69.32";
    IPS[1] = "10.176.69.33";
    IPS[2] = "10.176.69.34";
    IPS[3] = "10.176.69.35";
    
    int server_socket;
    if (MACHINE_ID != TOTAL_PROCESSES)
    {
        // create socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1)
        {
            fprintf(stderr,"Socket Creation Failed.\n");
            exit(1);
        }

        // set server address to bind to any IP
        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(7777);
        server_address.sin_addr.s_addr = INADDR_ANY;

        // binds the initialized server_address to the socket
        if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1)
        {
            fprintf(stderr,"Socket Bind Failed.\n");
            exit(1);
        }

        // listen for connections on server_sockets
        if (listen(server_socket, 4) == -1)
        {
            fprintf(stderr,"Socket Listen Failed.\n");
            exit(1);
        }

        // accepts a connection listening on server_socket and assigns the sockfd to network_sockets
        // sets open_socket at the accepted socket to true
        int i;
        for (i = TOTAL_PROCESSES - 1; i >= MACHINE_ID; i--)
        {
            network_sockets[i] = accept(server_socket, NULL, NULL);
            if (network_sockets[i] == -1)
            {
                fprintf(stderr,"Socket Accept Failed.\n");
                exit(1);
            }
            open_sockets[i] = 1;
        }
    }
    if (MACHINE_ID != 1)
    {   
        int i;
        // creates socket endpoints
        for (i = MACHINE_ID - 2; i >= 0; i--)
        {
            network_sockets[i] = socket(AF_INET, SOCK_STREAM, 0);
            if (network_sockets[i] == -1)
            {
                fprintf(stderr,"Socket Creation Failed.\n");
                exit(1);
            }
        }
        
        // connect to each IP other than the current machine's IP
        // sets open_socket at the accepted socket to true
        struct sockaddr_in server_address;        
        for (i = MACHINE_ID - 2; i >= 0; i--)
        {
            memset(&server_address, 0, sizeof(server_address));
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(7777);
            server_address.sin_addr.s_addr = inet_addr(IPS[i]);
            if (connect(network_sockets[i], (struct sockaddr *) &server_address, sizeof(server_address)) == -1)
            {
                fprintf(stderr,"Socket Connection Error.\n");
                exit(1);
            }
            open_sockets[i] = 1;
        }
    }

    // process 1 tells all other processes that all sockets have been connected
    if (MACHINE_ID == 1)
    {
        char message[BUFFER_SIZE] = "KxPsa080fIgySk7EJAclAXtra7V4XVqTLHluUo9IEIPFC3PJ0t";
        int i;
        for (i = 1; i < TOTAL_PROCESSES; i++)
        {
            send_message(network_sockets[i], message);
        }
    }

    // all other processes are told that all sockets are connected
    // and that they can continue their execution
    if (MACHINE_ID != 1)
    {
        char response[BUFFER_SIZE];
        size_t rcv_status = 0;
        while (rcv_status == 0)
        {
            memset(response, '\0', BUFFER_SIZE);
            rcv_status = recv(network_sockets[0], &response, sizeof(char) * BUFFER_SIZE, 0);
        }
        if (rcv_status == -1)
        {
            fprintf(stderr,"Socket Receive Failed.\n");
            exit(1);   
        }
        if (strcmp("KxPsa080fIgySk7EJAclAXtra7V4XVqTLHluUo9IEIPFC3PJ0t", response) != 0)
        {
            fprintf(stderr,"All Socket Connection Error.\n");
            exit(1);            
        }
    }
    printf("All Sockets are Connected.\n");

    pthread_t acc_th; // accept thread id
    pthread_t lis_th[TOTAL_PROCESSES]; // listen threads id

    // initialize accept thread struct
    struct s_accept *accept_th;
    accept_th = malloc(sizeof(struct s_accept));
    accept_th->MACHINE_ID = MACHINE_ID;
    accept_th->network_sockets = network_sockets;
    accept_th->open_sockets = open_sockets;

    // create accept thread
    if (pthread_create(&acc_th, NULL, &accepting_input, accept_th) != 0)
    {
        fprintf(stderr,"Create Accept Thread Failed.\n");
        exit(1);   
    }

    struct s_listen *listen_th[TOTAL_PROCESSES];

    // creates a listen thread for each open socket
    for (i = 0; i < TOTAL_PROCESSES; i++)
    {
        if (open_sockets[i] == 1)
        {
            // initialize listen thread struct
            listen_th[i] = malloc(sizeof(struct s_listen));
            memset(listen_th[i], '\0', sizeof(struct s_listen));
            listen_th[i]->ID = i;
            listen_th[i]->network_socket = network_sockets[i];
            listen_th[i]->open_sockets = open_sockets;

            // create listen thread
            if (pthread_create(&lis_th[i], NULL, &listen_messages, listen_th[i]) != 0)
            {
                fprintf(stderr,"Create Listen Thread Failed.\n");
                exit(1);
            }
        }
    }
    
    // wait for the accept thread and listen threads
    if (pthread_join(acc_th, NULL) != 0)
    {
        fprintf(stderr,"Wait Accept Thread Failed.\n");
        exit(1);
    }
    for (i = 0; i < TOTAL_PROCESSES; i++)
    {
        if (lis_th[i] != 0)
        {
            if (pthread_join(lis_th[i], NULL) != 0)
            {
                fprintf(stderr,"Wait Listen Thread Failed.\n");
                exit(1);
            }
        }
    }
}