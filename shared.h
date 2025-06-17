#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_PASSWORD_LENGTH 128
#define MAX_KEY_LENGTH (MAX_PASSWORD_LENGTH / 8)
#define MAX_ENCRYPTED_LENGTH 256

typedef struct {
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_KEY_LENGTH];
    char encrypted[MAX_ENCRYPTED_LENGTH];

    unsigned int password_length;
    unsigned int key_length;
    unsigned int encrypted_length;

    bool password_ready;      // New encrypted password available
    bool password_decrypted;  // Someone cracked the password
    int winner_id;            // Who won the round

    pthread_mutex_t mutex;
    pthread_cond_t cond;
} SharedData;

void shared_init(SharedData* data);

// Assignment-format logging utility functions
static inline void log_server_info(const char *fmt, ...) {
    va_list args;
    printf("%ld\t[SERVER]\t[INFO]\t", time(NULL));
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

static inline void log_server_ok(const char *fmt, ...) {
    va_list args;
    printf("%ld\t[SERVER]\t[OK]\t", time(NULL));
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

static inline void log_server_error(const char *fmt, ...) {
    va_list args;
    printf("%ld\t[SERVER]\t[ERROR]\t", time(NULL));
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

static inline void log_client_info(int id, const char *fmt, ...) {
    va_list args;
    printf("%ld\t[CLIENT #%d]\t[INFO]\t", time(NULL), id);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

// Submission function for clients to submit password guesses
int submit_password_attempt(SharedData* shared, int client_id, const char* guess, unsigned int length);

#endif // SHARED_H
