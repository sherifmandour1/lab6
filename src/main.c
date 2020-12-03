#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "http_server.h"
#include "log.h"

static int mainSockfd;
static Config mainConfig;
static volatile serverUp = 1;

void serverHandler()
{
    log_trace("server interreupted and shutting down");
    serverUp = 0;
    log_trace("Server is now taken down");
    
    exit(EXIT_SUCCESS);
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

void *handle_client(void *arg) {
    int cliSockfd = *((int *)arg);

    Request newRequest = http_server_receive_request(cliSockfd);
    sleep(15);
    Response newResponse = http_server_process_request(newRequest,mainConfig.relative_path);
    if(http_server_send_response(cliSockfd, newRequest, newResponse))
    {
        log_error("Sending failed");
    }
    http_server_client_cleanup(cliSockfd, newRequest, newResponse);
    return;
}

int main(int argc, char *argv[]) {
    int thread_count = 0;
    int thread_size = 5;
    pthread_t *threads = malloc(sizeof *threads * thread_size);

    ///// Do server work /////

    Config mainConfig = http_server_parse_arguments(argc, argv);

    if(mainConfig.port == NULL && mainConfig.relative_path == NULL)
    {
        log_error("Config was empty, exiting now");
        return EXIT_FAILURE;
    }
     if (strcmp(mainConfig.port, "invalid") == STRINGS_MATCH && strcmp(mainConfig.relative_path, "invalid") == STRINGS_MATCH)
    {
        log_error("Config contents were invalid, now exiting");
        return EXIT_SUCCESS;
    }
    
    mainSockfd = http_server_create(mainConfig);

    if(mainSockfd == HTTP_SERVER_BAD_SOCKET)
    {
        
        return EXIT_FAILURE;
    }

    signal(SIGINT, serverHandler);

    while(1)
    {

        if(serverUp)
        {
            break;
        }

        int* newSocket = add_socket(&threads, &thread_count, &thread_size);

        *newSocket = http_server_accept(mainSockfd);

        if(*newSocket == HTTP_SERVER_BAD_SOCKET)
        {
            continue;
        }

        pthread_t* newClientThread = add_thread(&threads, &thread_count, &thread_size);

        if(pthread_create(newClientThread, NULL, handle_client, newSocket))
        {
            log_error("Main did not create thread successfully");
        }
    }

    for(int i =0; i < thread_count; i++)
    {
        log_info("Joining thread %d",i);
        pthread_join(threads[i] ,NULL);
    }

    free(threads);

    return EXIT_SUCCESS;
}