#include "shared.h"
#include <string.h>

void shared_init(SharedData* data) {
    data->password_ready = false;
    data->password_decrypted = false;
    data->winner_id = -1;
    pthread_mutex_init(&data->mutex, NULL);
    pthread_cond_init(&data->cond, NULL);
}

// Client calls this to try to claim a win
int submit_password_attempt(SharedData* shared, int client_id, const char* guess, unsigned int length) {
    int result = 0;
    pthread_mutex_lock(&shared->mutex);
    if (shared->password_ready && !shared->password_decrypted) {
        if (length == shared->password_length && memcmp(guess, shared->password, length) == 0) {
            shared->password_decrypted = true;
            shared->winner_id = client_id;
            pthread_cond_broadcast(&shared->cond);
            result = 1; // correct
        } else {
            log_server_error("Wrong password received from client #%d(%.*s), should be (%.*s)",
                client_id, length, guess, shared->password_length, shared->password);
            result = 0; // wrong password
        }
    } else {
        // Already solved or not open for answers
        log_server_error("Wrong password received from client #%d(%.*s), should be (%.*s)",
            client_id, length, guess, shared->password_length, shared->password);
        result = 0;
    }
    pthread_mutex_unlock(&shared->mutex);
    return result;
}
