// logger.c
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "logger.h" // Include the header to match the definition

void write_log(const char *message) {
    FILE *fp = fopen("logs/server_log.txt", "a");
    if (fp == NULL) return;

    time_t now;
    time(&now);
    char *date = ctime(&now);
    date[strcspn(date, "\n")] = 0; // Remove newline

    fprintf(fp, "[%s] SYSTEM: %s\n", date, message);
    fclose(fp);
}