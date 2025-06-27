#include <pthread.h>
#include <mta_rand.h>
#include <mta_crypt.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "shared.h"
#include "client.h"

static bool is_printable(const char* buf, unsigned int len) {
    for (unsigned int i = 0; i < len; i++)
        if (!isprint((unsigned char)buf[i]))
            return false;
    return true;
}

void* decrypter_thread_func(void* arg) {
    ClientArgs* args = (ClientArgs*)arg;
    SharedData* shared = args->shared;
    int id = args->client_id;
    char attempt_key[MAX_KEY_LENGTH];
    char attempt_plain[MAX_PASSWORD_LENGTH];
    unsigned int attempt_plain_len;
    unsigned int iterations = 0;

    while (1) {
        // Wait for a new password to crack
        pthread_mutex_lock(&shared->mutex);
        while (!shared->password_ready && !shared->password_decrypted)
            pthread_cond_wait(&shared->cond, &shared->mutex);

        unsigned int encrypted_length = shared->encrypted_length;
        unsigned int key_length = shared->key_length;
        unsigned int password_length = shared->password_length;
        char encrypted[MAX_ENCRYPTED_LENGTH];
        memcpy(encrypted, shared->encrypted, encrypted_length);

        pthread_mutex_unlock(&shared->mutex);

        iterations = 0;
        while (1) {
            MTA_get_rand_data(attempt_key, key_length);

            if (MTA_decrypt(attempt_key, key_length, encrypted, encrypted_length, attempt_plain, &attempt_plain_len) == MTA_CRYPT_RET_OK
                && attempt_plain_len == password_length && is_printable(attempt_plain, password_length)) {

                int res = submit_password_attempt(shared, id, attempt_plain, password_length);

                if (res == 1) {
                    log_client_info(id, "After decryption(%.*s), key guessed(%.*s), sending to server after %u iterations",
                        encrypted_length, encrypted, key_length, attempt_key, iterations);
                    break;
                } else {
                    break;
                }
            }

            pthread_mutex_lock(&shared->mutex);
            bool done = (!shared->password_ready || shared->password_decrypted);
            pthread_mutex_unlock(&shared->mutex);
            if (done)
                break;

            iterations++;
        }
    }
    return NULL;
}
