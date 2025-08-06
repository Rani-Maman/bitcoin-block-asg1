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
#define MAX_DECRYPTERS 32
#define PIPE_PATH "/mnt/mta/encrypter_pipe"
#define CONFIG_PATH "/mnt/mta/mtacrypt.conf"
#define LOG_PATH "/var/log/encrypter.log"

typedef struct {
    char pipe_name[256];
    int pipe_fd;
    int active;
} DecrypterInfo;

typedef struct {
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_KEY_LENGTH];
    char encrypted[MAX_ENCRYPTED_LENGTH];
    unsigned int password_length;
    unsigned int key_length;
    unsigned int encrypted_length;
    unsigned int password_id;
    int timeout;
} EncrypterData;

static DecrypterInfo decrypters[MAX_DECRYPTERS];
static int num_decrypters = 0;
static EncrypterData enc_data;
static FILE* log_file = NULL;

static void log_server_info(const char *fmt, ...) {
    va_list args;
    time_t now = time(NULL);
    
    printf("%ld\t[SERVER]\t[INFO]\t", now);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    
    if (log_file) {
        fprintf(log_file, "%ld\t[SERVER]\t[INFO]\t", now);
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

static void log_server_ok(const char *fmt, ...) {
    va_list args;
    time_t now = time(NULL);
    
    printf("%ld\t[SERVER]\t[OK]\t", now);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    
    if (log_file) {
        fprintf(log_file, "%ld\t[SERVER]\t[OK]\t", now);
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

static void log_server_error(const char *fmt, ...) {
    va_list args;
    time_t now = time(NULL);
    
    printf("%ld\t[SERVER]\t[ERROR]\t", now);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    
    if (log_file) {
        fprintf(log_file, "%ld\t[SERVER]\t[ERROR]\t", now);
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

static int read_config_file() {
    FILE* config_file = fopen(CONFIG_PATH, "r");
    if (!config_file) {
        log_server_error("Failed to open config file %s", CONFIG_PATH);
        return -1;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), config_file)) {
        if (strncmp(line, "password_length=", 16) == 0) {
            enc_data.password_length = atoi(line + 16);
        } else if (strncmp(line, "timeout=", 8) == 0) {
            enc_data.timeout = atoi(line + 8);
        }
    }
    
    fclose(config_file);
    
    if (enc_data.password_length <= 0 || enc_data.password_length % 8 != 0) {
        log_server_error("Invalid password length in config: %d", enc_data.password_length);
        return -1;
    }
    
    enc_data.key_length = enc_data.password_length / 8;
    if (enc_data.timeout <= 0) {
        enc_data.timeout = 30;
    }
    
    log_server_info("Config loaded: password_length=%d, timeout=%d", 
                   enc_data.password_length, enc_data.timeout);
    return 0;
}

static void generate_printable_password(char *password, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        char c;
        do {
            c = MTA_get_rand_char();
        } while (!isprint((unsigned char)c));
        password[i] = c;
    }
}

static void generate_new_password() {
    generate_printable_password(enc_data.password, enc_data.password_length);
    MTA_get_rand_data(enc_data.key, enc_data.key_length);
    
    unsigned int encrypted_length;
    MTA_encrypt(enc_data.key, enc_data.key_length,
                enc_data.password, enc_data.password_length,
                enc_data.encrypted, &encrypted_length);
    
    enc_data.encrypted_length = encrypted_length;
    enc_data.password_id++;
    
    log_server_info("New password generated: %.*s, key: %.*s, After encryption: %.*s",
                   enc_data.password_length, enc_data.password,
                   enc_data.key_length, enc_data.key,
                   enc_data.encrypted_length, enc_data.encrypted);
}

static void broadcast_password_to_decrypters() {
    char message[512];
    int msg_len = snprintf(message, sizeof(message), "PASSWORD:%d:%.*s",
                          enc_data.password_id,
                          enc_data.encrypted_length, enc_data.encrypted);
    
    for (int i = 0; i < num_decrypters; i++) {
        if (decrypters[i].active && decrypters[i].pipe_fd >= 0) {
            write(decrypters[i].pipe_fd, message, msg_len);
        }
    }
}

static int add_decrypter(const char* pipe_name) {
    if (num_decrypters >= MAX_DECRYPTERS) {
        return -1;
    }
    
    strcpy(decrypters[num_decrypters].pipe_name, pipe_name);
    decrypters[num_decrypters].pipe_fd = open(pipe_name, O_WRONLY | O_NONBLOCK);
    if (decrypters[num_decrypters].pipe_fd < 0) {
        log_server_error("Failed to open decrypter pipe: %s", pipe_name);
        return -1;
    }
    
    decrypters[num_decrypters].active = 1;
    num_decrypters++;
    
    log_server_info("Decrypter added: %s (total: %d)", pipe_name, num_decrypters);
    return 0;
}

static void handle_subscription_message(const char* message) {
    if (strncmp(message, "SUBSCRIBE:", 10) == 0) {
        const char* pipe_name = message + 10;
        char full_pipe_path[512];
        snprintf(full_pipe_path, sizeof(full_pipe_path), "/mnt/mta/%s", pipe_name);
        
        if (add_decrypter(full_pipe_path) == 0) {
            char response[512];
            int pipe_fd = open(full_pipe_path, O_WRONLY | O_NONBLOCK);
            if (pipe_fd >= 0) {
                int resp_len = snprintf(response, sizeof(response), "PASSWORD:%d:%.*s",
                                       enc_data.password_id,
                                       enc_data.encrypted_length, enc_data.encrypted);
                write(pipe_fd, response, resp_len);
                close(pipe_fd);
            }
        }
    }
}

static void handle_decrypted_password(const char* message) {
    if (strncmp(message, "DECRYPTED:", 10) == 0) {
        const char* password_data = message + 10;
        char* colon_pos = strchr(password_data, ':');
        if (!colon_pos) return;
        
        int client_id = atoi(password_data);
        const char* attempt = colon_pos + 1;
        unsigned int attempt_len = strlen(attempt);
        
        if (attempt_len == enc_data.password_length &&
            memcmp(attempt, enc_data.password, attempt_len) == 0) {
            
            log_server_ok("Password decrypted successfully by client %d, received(%.*s), is (%.*s)",
                         client_id, attempt_len, attempt, enc_data.password_length, enc_data.password);
            
            generate_new_password();
            broadcast_password_to_decrypters();
        } else {
            log_server_error("Wrong password received from client #%d(%.*s), should be (%.*s)",
                           client_id, attempt_len, attempt, enc_data.password_length, enc_data.password);
        }
    }
}

static void check_for_messages(int pipe_fd) {
    char buffer[1024];
    int bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        char* line = strtok(buffer, "\n");
        while (line) {
            if (strncmp(line, "SUBSCRIBE:", 10) == 0) {
                handle_subscription_message(line);
            } else if (strncmp(line, "DECRYPTED:", 10) == 0) {
                handle_decrypted_password(line);
            }
            line = strtok(NULL, "\n");
        }
    }
}

int main() {
    log_file = fopen(LOG_PATH, "a");
    if (!log_file) {
        fprintf(stderr, "Warning: Could not open log file %s\n", LOG_PATH);
    }
    
    if (MTA_crypt_init() != MTA_CRYPT_RET_OK) {
        log_server_error("Failed to initialize crypto library");
        exit(EXIT_FAILURE);
    }
    
    if (read_config_file() < 0) {
        exit(EXIT_FAILURE);
    }
    
    unlink(PIPE_PATH);
    if (mkfifo(PIPE_PATH, 0666) < 0) {
        log_server_error("Failed to create encrypter pipe");
        exit(EXIT_FAILURE);
    }
    
    int pipe_fd = open(PIPE_PATH, O_RDONLY | O_NONBLOCK);
    if (pipe_fd < 0) {
        log_server_error("Failed to open encrypter pipe");
        exit(EXIT_FAILURE);
    }
    
    generate_new_password();
    
    time_t last_password_time = time(NULL);
    
    log_server_info("Encrypter started, waiting for decrypters...");
    
    while (1) {
        check_for_messages(pipe_fd);
        
        time_t current_time = time(NULL);
        if (current_time - last_password_time >= enc_data.timeout) {
            log_server_info("Timeout expired, generating new password");
            generate_new_password();
            broadcast_password_to_decrypters();
            last_password_time = current_time;
        }
        
        usleep(100000);
    }
    
    close(pipe_fd);
    if (log_file) {
        fclose(log_file);
    }
    
    return 0;
}
