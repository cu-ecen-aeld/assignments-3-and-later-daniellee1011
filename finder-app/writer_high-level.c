#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

int main (int argc, char *argv[]) {
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
    FILE *file = fopen(filename, "w");
    if (!file) {
        syslog(LOG_ERR, "Failed to open file %s for writing", filename);
        perror("Error");
        closelog();
        return 1;
    }

    // Write content to the file
    if (fprintf(file, "%s", content) < 0) {
        syslog(LOG_ERR, "Failed to write to file %s", filename);
        perror("Error");
        closelog();
        return 1;
    }

    // Log success
    syslog(LOG_DEBUG, "Writing \"%s\" to %s", content, filename);

    // Clean up
    fclose(file);
    closelog();
    return 0;
}
