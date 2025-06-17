#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mta_crypt.h>
#include <pthread.h>
#include "shared.h"
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
    int num_decrypters = -1;
    int password_length = -1;
    int timeout = 0;

    // Simple manual parsing
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i+1 < argc) {
            num_decrypters = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-l") == 0 && i+1 < argc) {
            password_length = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i+1 < argc) {
            timeout = atoi(argv[++i]);
        }
    }

    if (num_decrypters <= 0 || password_length <= 0 || (password_length % 8 != 0)) {
        fprintf(stderr, "Missing num of decrypters\n");
        fprintf(stderr, "Usage: encrypt.out [-t|--timeout seconds] <-n|--num-of-decrypters <number>> <-l|--password-length <length>>\n");
        exit(EXIT_FAILURE);
    }
    unsigned int key_length = password_length / 8;
    if (key_length > MAX_KEY_LENGTH) {
        fprintf(stderr, "Error: key_length exceeds max allowed size. Use smaller password_length.\n");
        exit(EXIT_FAILURE);
    }

    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Error: Failed to initialize crypto library.\n");
        exit(EXIT_FAILURE);
    }

    SharedData shared;
    shared_init(&shared);
    shared.password_length = password_length;
    shared.key_length = key_length;

    // Encrypter thread
    pthread_t encrypter_thread;
    if (pthread_create(&encrypter_thread, NULL, encrypter_thread_func, &shared) != 0) {
        fprintf(stderr, "Error: Failed to create encrypter thread.\n");
        exit(EXIT_FAILURE);
    }

    // Decrypter threads
    pthread_t* decrypter_threads = malloc(sizeof(pthread_t) * num_decrypters);
    ClientArgs* client_args = malloc(sizeof(ClientArgs) * num_decrypters);
    for (int i = 0; i < num_decrypters; i++) {
        client_args[i].shared = &shared;
        client_args[i].client_id = i;
        if (pthread_create(&decrypter_threads[i], NULL, decrypter_thread_func, &client_args[i]) != 0) {
            fprintf(stderr, "Error: Failed to create decrypter thread %d.\n", i);
            exit(EXIT_FAILURE);
        }
    }

    pthread_join(encrypter_thread, NULL);
    for (int i = 0; i < num_decrypters; i++) {
        pthread_cancel(decrypter_threads[i]);
    }
    free(decrypter_threads);
    free(client_args);

    return 0;
}
