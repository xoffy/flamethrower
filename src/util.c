
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

int u_quiet = 0;

#define COL_RED     "\x1b[31m"
#define COL_GREEN   "\x1b[32m"
#define COL_YELLOW  "\x1b[33m"
#define COL_BLUE    "\x1b[34m"
#define COL_MAGENTA "\x1b[35m"
#define COL_CYAN    "\x1b[36m"
#define COL_RESET   "\x1b[0m"

void u_debug(const char *fmt, ...) {
    va_list args;
    
    if (u_quiet) {
        return;
    }
    
    printf("[" COL_BLUE "DBG" COL_RESET "] ");
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

void u_message(const char *fmt, ...) {
    va_list args;
    
    if (u_quiet) {
        return;
    }
    
    printf("[" COL_GREEN "MSG" COL_RESET "] ");
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

void u_error(const char *fmt, ...) {
    va_list args;
    
    printf("[" COL_RED "ERR" COL_RESET "] ");
    
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
}

void u_get_file_base(char *base, const char *path) {
    char *dot;
    
    strcpy(base, path);
    dot = strrchr(base, '.');
    *dot = '\0';
}

const char *u_get_file_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    if (dot == NULL || dot == path) {
        return NULL;
    }
    return dot + 1;
}

