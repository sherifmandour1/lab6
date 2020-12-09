#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include "http_server.h"
#include "log.h"

#define STRINGS_MATCH 0

//main socket to be user later in main
static int mainSockfd;
//main config to be filled out and used later in main
static Config mainConfig;
//var/condition for keeping the server running
volatile sig_atomic_t serverUp = true;

//handles the signals
void serverHandler()
{
    log_trace("server interreupted and shutting down");
    //takes down the server
    serverUp = false;
    log_trace("Server is now taken down");
    //cleaning up after the server
    http_server_cleanup(mainSockfd);

}

pthread_t *add_thread(pthread_t *threads[], int *thread_count, int *thread_size) {
    // If we don't have room, add more space in the threads array
    if (*thread_count == *thread_size) {
        *thread_size *= 2; // Double it

        *threads = realloc(*threads, sizeof(**threads) * (*thread_size));
    }

    pthread_t *new_thread = &(*threads)[*thread_count];
    (*thread_count)++;
    return new_thread;
}


// //this functions is responsible for creating sockets to connect to the clients
// int* add_socket(int *sockets[], int *socket_count, int *socket_size) {
//     // If we don't have room, add more space in the threads array
//     if (*socket_count == *socket_size) {
//         *socket_size *= 2; // Double it

//         *sockets = realloc(*sockets, sizeof(**sockets) * (*socket_size));
//     }

//     int *new_socket = &(*sockets)[*socket_size];
//     (*socket_count)++;
//     return new_socket;
// }

//handling client requests
void *handle_client(void *arg) {
    int* cliSockfd = ((int *)arg);
log_trace("handling\n");
    Request newRequest = http_server_receive_request(*cliSockfd);
    sleep(15);
    Response newResponse = http_server_process_request(newRequest,mainConfig.relative_path);
    if(http_server_send_response(*cliSockfd, newResponse))
    {
        log_error("Sending failed");
    }
            
    
    http_server_client_cleanup(*cliSockfd, newRequest, newResponse);
    free(cliSockfd);
    return NULL;
}

int main(int argc, char *argv[]) {

    //initializing the socket and thread numbers 
    int thread_count = 0;
    int thread_size = 5;
    //int socket_count = 0;
    int socket_size = 5;


    //this allocats enough meomory for all the threads and sockets 
    pthread_t *threads = malloc(sizeof *threads * thread_size);
    // int* mySockets = malloc(sizeof *mySockets * socket_size);
    ///// Do server work /////

    mainConfig = http_server_parse_arguments(argc, argv);
    if(mainConfig.port == NULL && mainConfig.relative_path == NULL)
    {
        log_error("Config was empty, exiting now");
        return EXIT_FAILURE;
    }
     else if (strcmp(mainConfig.port, "invalid") == STRINGS_MATCH && strcmp(mainConfig.relative_path, "invalid") == STRINGS_MATCH)
    {
        log_error("Config contents were invalid, now exiting");
        return EXIT_SUCCESS;
    }
    //creating the server and loading the main socket
    mainSockfd = http_server_create(mainConfig);
    //error checking to see that the socket loading was successful
    if(mainSockfd == HTTP_SERVER_BAD_SOCKET)
    {
        log_error("Socket failed in main");
        return EXIT_FAILURE;
    }
    //signals being caught, either ctrl-c, or the signal ignored by receiving binary data 
    signal(SIGINT, serverHandler);
    // signal(SIGINT, SIG_IGN);
    // while loop to carry out all the action of process
    while(serverUp)
    {
        //create new socket
        int* newSocket = malloc(sizeof(int) * 1);//add_socket(&mySockets, &socket_count, &socket_size);
        //attemp to accept the new socket
        *newSocket = http_server_accept(mainSockfd);
        if(*newSocket == HTTP_SERVER_BAD_SOCKET)
        {
            free(newSocket);
            continue;
        }
        //adding a thread 
        pthread_t* clientThread = add_thread(&threads, &thread_count, &thread_size);
        //thread creation and connecting it to a socket
        if(pthread_create(clientThread, NULL, handle_client, newSocket))
        {
            free(newSocket);
            log_error("Main did not create thread successfully");
            continue;
        }
        log_trace("made it\n");
    }

    for(int i =0; i < thread_count; i++)
    {
        log_info("Joining thread %d",i);
        pthread_join(threads[i] ,NULL);
    }

    //free the threads and sockets
    free(threads);
    // free(mySockets);
    return EXIT_SUCCESS;
}