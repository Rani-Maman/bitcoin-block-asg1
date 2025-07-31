#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <mta_rand.h>
#include <mta_crypt.h>

#define MAX_PASSWORD_LENGTH 128
#define MAX_KEY_LENGTH (MAX_PASSWORD_LENGTH / 8)
#define MAX_ENCRYPTED_LENGTH 256
#define ENCRYPTER_PIPE_PATH "/mnt/mta/encrypter_pipe"
#define LOG_PATH "/var/log/decrypter.log"

typedef struct {
    char encrypted[MAX_ENCRYPTED_LENGTH];
    unsigned int encrypted_length;
    unsigned int password_length;
    unsigned int key_length;
    unsigned int password_id;
    int decrypter_id;
    char my_pipe_path[256];
    int my_pipe_fd;
    int encrypter_pipe_fd;
} DecrypterData;

static DecrypterData dec_data;
static FILE* log_file = NULL;

static void log_client_info(int id, const char *fmt, ...) {
    va_list args;
    time_t now = time(NULL);
    
    printf("%ld\t[CLIENT #%d]\t[INFO]\t", now, id);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    
    if (log_file) {
        fprintf(log_file, "%ld\t[CLIENT #%d]\t[INFO]\t", now, id);
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

static int is_printable(const char* buf, unsigned int len) {
    for (unsigned int i = 0; i < len; i++)
        if (!isprint((unsigned char)buf[i]))
            return 0;
    return 1;
}

static int find_next_available_id() {
    for (int id = 1; id <= 999; id++) {
        char pipe_path[256];
        snprintf(pipe_path, sizeof(pipe_path), "/mnt/mta/decrypter_pipe_%d", id);
        
        if (access(pipe_path, F_OK) != 0) {
            return id;
        }
    }
    return -1;
}

static int create_my_pipe() {
    dec_data.decrypter_id = find_next_available_id();
    if (dec_data.decrypter_id < 0) {
        fprintf(stderr, "No available decrypter ID found\n");
        return -1;
    }
    
    snprintf(dec_data.my_pipe_path, sizeof(dec_data.my_pipe_path), 
             "/mnt/mta/decrypter_pipe_%d", dec_data.decrypter_id);
    
    unlink(dec_data.my_pipe_path);
    if (mkfifo(dec_data.my_pipe_path, 0666) < 0) {
        fprintf(stderr, "Failed to create decrypter pipe: %s\n", dec_data.my_pipe_path);
        return -1;
    }
    
    dec_data.my_pipe_fd = open(dec_data.my_pipe_path, O_RDONLY | O_NONBLOCK);
    if (dec_data.my_pipe_fd < 0) {
        fprintf(stderr, "Failed to open decrypter pipe: %s\n", dec_data.my_pipe_path);
        return -1;
    }
    
    log_client_info(dec_data.decrypter_id, "Created pipe: %s", dec_data.my_pipe_path);
    return 0;
}

static int send_subscription() {
    dec_data.encrypter_pipe_fd = open(ENCRYPTER_PIPE_PATH, O_WRONLY);
    if (dec_data.encrypter_pipe_fd < 0) {
        log_client_info(dec_data.decrypter_id, "Failed to open encrypter pipe, retrying...");
        return -1;
    }
    
    char subscription_msg[512];
    int msg_len = snprintf(subscription_msg, sizeof(subscription_msg), 
                          "SUBSCRIBE:decrypter_pipe_%d\n", dec_data.decrypter_id);
    
    if (write(dec_data.encrypter_pipe_fd, subscription_msg, msg_len) < 0) {
        log_client_info(dec_data.decrypter_id, "Failed to send subscription");
        close(dec_data.encrypter_pipe_fd);
        return -1;
    }
    
    close(dec_data.encrypter_pipe_fd);
    log_client_info(dec_data.decrypter_id, "Subscription sent");
    return 0;
}

static int check_for_new_password() {
    char buffer[1024];
    int bytes_read = read(dec_data.my_pipe_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        if (strncmp(buffer, "PASSWORD:", 9) == 0) {
            char* data = buffer + 9;
            char* colon1 = strchr(data, ':');
            if (!colon1) return 0;
            
            unsigned int new_password_id = atoi(data);
            char* encrypted_data = colon1 + 1;
            
            if (new_password_id != dec_data.password_id) {
                dec_data.password_id = new_password_id;
                dec_data.encrypted_length = strlen(encrypted_data);
                memcpy(dec_data.encrypted, encrypted_data, dec_data.encrypted_length);
                
                dec_data.password_length = dec_data.encrypted_length;
                dec_data.key_length = dec_data.password_length / 8;
                
                log_client_info(dec_data.decrypter_id, "Received new password (ID: %d): %.*s",
                               dec_data.password_id, dec_data.encrypted_length, dec_data.encrypted);
                return 1;
            }
        }
    }
    return 0;
}

static void send_decrypted_password(const char* password, unsigned int length, unsigned int iterations) {
    int encrypter_fd = open(ENCRYPTER_PIPE_PATH, O_WRONLY | O_NONBLOCK);
    if (encrypter_fd >= 0) {
        char message[512];
        int msg_len = snprintf(message, sizeof(message), "DECRYPTED:%d:%.*s\n",
                              dec_data.decrypter_id, length, password);
        write(encrypter_fd, message, msg_len);
        close(encrypter_fd);
        
        log_client_info(dec_data.decrypter_id, 
                       "After decryption(%.*s), key guessed, sending to server after %u iterations",
                       dec_data.encrypted_length, dec_data.encrypted, iterations);
    }
}

static void decrypt_loop() {
    char attempt_key[MAX_KEY_LENGTH];
    char attempt_plain[MAX_PASSWORD_LENGTH];
    unsigned int attempt_plain_len;
    unsigned int iterations = 0;
    unsigned int current_password_id = 0;
    
    while (1) {
        if (check_for_new_password()) {
            current_password_id = dec_data.password_id;
            iterations = 0;
            log_client_info(dec_data.decrypter_id, "Starting decryption for password ID: %d", current_password_id);
        }
        
        if (current_password_id == 0) {
            usleep(100000);
            continue;
        }
        
        MTA_get_rand_data(attempt_key, dec_data.key_length);
        
        if (MTA_decrypt(attempt_key, dec_data.key_length, 
                       dec_data.encrypted, dec_data.encrypted_length,
                       attempt_plain, &attempt_plain_len) == MTA_CRYPT_RET_OK
            && attempt_plain_len == dec_data.password_length 
            && is_printable(attempt_plain, dec_data.password_length)) {
            
            send_decrypted_password(attempt_plain, attempt_plain_len, iterations);
        }
        
        iterations++;
    }
}

int main() {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "/var/log/decrypter_%d.log", getpid());
    log_file = fopen(log_path, "a");
    if (!log_file) {
        fprintf(stderr, "Warning: Could not open log file %s\n", log_path);
    }
    
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        fprintf(stderr, "Failed to initialize crypto library\n");
        exit(EXIT_FAILURE);
    }
    
    if (create_my_pipe() < 0) {
        exit(EXIT_FAILURE);
    }
    
    while (send_subscription() < 0) {
        sleep(1);
    }
    
    log_client_info(dec_data.decrypter_id, "Decrypter started, waiting for passwords...");
    
    decrypt_loop();
    
    close(dec_data.my_pipe_fd);
    unlink(dec_data.my_pipe_path);
    if (log_file) {
        fclose(log_file);
    }
    
    return 0;
}
