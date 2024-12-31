#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    // Open syslog for logging
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);

    // Validate arguments
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid number of arguments: expected 2, got %d", argc - 1);
        fprintf(stderr, "Usage: %s <filename> <string>\n", argv[0]);
        closelog();
        return 1;
    }

    const char *filename = argv[1];
    const char *content = argv[2];

    // Open the file for writing
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to open file %s: %s", filename, strerror(errno));
        perror("Error");
        closelog();
        return 1;
    }

    // Write content to the file
    ssize_t bytes_written = write(fd, content, strlen(content));
    if (bytes_written == -1) {
        syslog(LOG_ERR, "Failed to write to file %s: %s", filename, strerror(errno));
        perror("Error");
        close(fd);
        closelog();
        return 1;
    }

    // Log Success
    syslog(LOG_DEBUG, "Writing \"%s\" to %s", content, filename);

    // Clean up
    close(fd);
    closelog();
    return 0;
}
