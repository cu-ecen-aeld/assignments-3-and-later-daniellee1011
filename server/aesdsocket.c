#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_socket;
struct sockaddr_in server_addr;

void handle_signal(int signo) {
    syslog(LOG_INFO, "Caught signal, exiting");

    // Clean up: Close socket and delete the file
    if (server_socket != -1) {
        close(server_socket);
    }

    // Remove the temporary file
    if (remove(FILE_PATH) == 0) {
        syslog(LOG_INFO, "Deleted file %s", FILE_PATH);
    } else {
        syslog(LOG_INFO, "Failed to delete file %s", FILE_PATH);
    }

    closelog();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // Register signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Chec if the daemon mode (-d flag) is set
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        // Run in daemon mode
        if (daemon(0, 0) == -1) {
            syslog(LOG_ERR, "Failed to run in daemon mode");
            return -1;
        }
    }

    syslog(LOG_INFO, "Starting aesdsocket...");

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Socket creation failed");
        return -1;
    }
	
	// Allow the socket to reuse the port
	int opt = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
	    syslog(LOG_ERR, "setsockopt failed");
	    close(server_socket);
	    return -1;
	}

    // Bind the socket to port 9000
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9000);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Socket bind failed");
        close(server_socket);
        return -1;
    }

    // Start listening for connections
    if (listen(server_socket, 10) == -1) {
        syslog(LOG_ERR, "Listen failed");
        close(server_socket);
        return -1;
    }

    syslog(LOG_INFO, "aesdsocket is listening on port 9000");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket;

    while (1) {
        // Accept a new connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        // Log the accepted connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Keep the connection open to hanlde multiple messages
        FILE *file = fopen(FILE_PATH, "a+");
        if (!file) {
            syslog(LOG_ERR, "Failed to open file");
            close(client_socket);
            return -1;
        }

        // Handle communication with the client
        char buffer[1024];
        int bytes_received;

        // Receive data from client
        while ((bytes_received = recv(client_socket, buffer, sizeof(buffer) -1, 0)) > 0) {
            buffer[bytes_received] = '\0'; // Null-terminate the buffer

            // Write the received data to the file
            fprintf(file, "%s", buffer);
            fflush(file);

            // Check the newline character and send file content back to client
            if (strchr(buffer, '\n')) {
                rewind(file);
                while (fgets(buffer, sizeof(buffer), file)) {
                    send(client_socket, buffer, strlen(buffer), 0);
                }
                fflush(file);
            }
        }

        fclose(file);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_socket);
    }

    return 0;
}
