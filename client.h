#ifndef CLIENT_H
#define CLIENT_H

#include "shared.h"

typedef struct {
    SharedData* shared;
    int client_id;
} ClientArgs;

void* decrypter_thread_func(void* arg);

#endif // CLIENT_H
