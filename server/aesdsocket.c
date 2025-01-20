#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>

#define FILE_PATH "/var/tmp/aesdsocketdata"
#define PORT 9000
#define BUFFER_SIZE 1024

int server_socket;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
int shutdown_requested = 0;

// Struct for linked list to manage threads
struct thread_node {
    pthread_t thread_id;
    struct thread_node *next;
};

struct thread_node *thread_list_head = NULL;

// Function to append a timestamp to the file
void append_timestamp() {
    time_t now = time(NULL);
    char time_str[100];
    strftime(time_str, sizeof(time_str), "timestamp: %a, %d %b %Y %H:%M:%S\n", localtime(&now));

    pthread_mutex_lock(&file_mutex);
    FILE *file = fopen(FILE_PATH, "a");
    if (file) {
        fputs(time_str, file);
        fclose(file);
    }
    pthread_mutex_unlock(&file_mutex);
}

// Signal handler using struct sigaction
void handle_signal(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        shutdown_requested = 1;

        if (server_socket != -1) {
            close(server_socket);
        }

        if (remove(FILE_PATH) == 0) {
            syslog(LOG_INFO, "Deleted file %s", FILE_PATH);
        } else {
            syslog(LOG_ERR, "Failed to delete file %s", FILE_PATH);
        }

        closelog();
    } else if (signo == SIGUSR1) {
        // SIGUSR1 triggers timestamp append
        // append_timestamp();
    }
}

// Thread function to handle client connections
void *connection_handler(void *client_socket) {
    int sockfd = *(int *)client_socket;
    free(client_socket);

    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        pthread_mutex_lock(&file_mutex);

        file = fopen(FILE_PATH, "a+");
        if (!file) {
            syslog(LOG_ERR, "Failed to open file");
            pthread_mutex_unlock(&file_mutex);
            close(sockfd);
            return NULL;
        }

        fprintf(file, "%s", buffer);
        fflush(file);
        fsync(fileno(file)); // Ensure the write is flushed
        fclose(file);

        pthread_mutex_unlock(&file_mutex);

        if (strchr(buffer, '\n')) {
            pthread_mutex_lock(&file_mutex);
            file = fopen(FILE_PATH, "r");
            if (file) {
                while (fgets(buffer, sizeof(buffer), file)) {
                    send(sockfd, buffer, strlen(buffer), 0);
                }
                fclose(file);
            }
            pthread_mutex_unlock(&file_mutex);
        }
    }

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = handle_signal;
    sigaction(SIGINT, &new_action, NULL);
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGUSR1, &new_action, NULL); // Register SIGUSR1 for timestamp appending

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        if (daemon(0, 0) == -1) {
            syslog(LOG_ERR, "Failed to run in daemon mode");
            return -1;
        }
    }

    syslog(LOG_INFO, "Starting aesdsocket...");

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        syslog(LOG_ERR, "setsockopt failed");
        close(server_socket);
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Socket bind failed");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, 10) == -1) {
        syslog(LOG_ERR, "Listen failed");
        close(server_socket);
        return -1;
    }

    syslog(LOG_INFO, "aesdsocket is listening on port %d", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (!shutdown_requested) {
        int *client_socket = malloc(sizeof(int));
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (*client_socket == -1) {
            free(client_socket);
            if (shutdown_requested) break;
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        pthread_t thread;
        pthread_create(&thread, NULL, connection_handler, client_socket);

        struct thread_node *new_node = malloc(sizeof(struct thread_node));
        new_node->thread_id = thread;
        new_node->next = thread_list_head;
        thread_list_head = new_node;
    }

    struct thread_node *current = thread_list_head;
    while (current) {
        pthread_join(current->thread_id, NULL);
        struct thread_node *next = current->next;
        free(current);
        current = next;
    }

    pthread_mutex_destroy(&file_mutex);
    closelog();

    return 0;
}
