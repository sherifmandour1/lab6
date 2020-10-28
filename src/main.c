#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "http_server.h"
#include "log.h"

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

void *handle_client(void *arg) {}

int main(int argc, char *argv[]) {
    int thread_count = 0;
    int thread_size = 5;
    pthread_t *threads = malloc(sizeof *threads * thread_size);

    ///// Do server work /////

    free(threads);

    return EXIT_SUCCESS;
}