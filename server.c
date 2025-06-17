#include <pthread.h>
#include <mta_rand.h>
#include <mta_crypt.h>
#include <ctype.h>
#include <stdio.h>
#include "shared.h"
#include "server.h"

void generate_printable_password(char *password, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint((unsigned char)c));
        password[i] = c;
    }
}

void* encrypter_thread_func(void* arg) {
    SharedData* shared = (SharedData*)arg;

    while (1) {
        pthread_mutex_lock(&shared->mutex);

        generate_printable_password(shared->password, shared->password_length);
        MTA_get_rand_data(shared->key, shared->key_length);

        unsigned int encrypted_length;
        MTA_encrypt(shared->key, shared->key_length,
                    shared->password, shared->password_length,
                    shared->encrypted, &encrypted_length);

        shared->encrypted_length = encrypted_length;
        shared->password_ready = true;
        shared->password_decrypted = false;
        shared->winner_id = -1;

        log_server_info("New password generated: %.*s, key: %.*s, After encryption: %.*s",
            shared->password_length, shared->password,
            shared->key_length, shared->key,
            shared->encrypted_length, shared->encrypted);

        pthread_cond_broadcast(&shared->cond);

        // Wait for a decrypter to set password_decrypted (someone cracked it)
        while (!shared->password_decrypted) {
            pthread_cond_wait(&shared->cond, &shared->mutex);
        }

        log_server_ok("Password decrypted successfully by client %d, received(%.*s), is (%.*s)",
            shared->winner_id, shared->password_length, shared->password, shared->password_length, shared->password);

        shared->password_ready = false;
        pthread_mutex_unlock(&shared->mutex);
    }
    pthread_exit(NULL);
}
