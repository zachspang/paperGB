#include "common.h"

void LOG(const char* txt, ...) {
    va_list args;
    va_start(args, txt);
    vprintf(txt, args);
    va_end(args);
    printf("\n");
}

void LOG_WARN(const char* txt, ...) {
    printf("WARNING: ");
    va_list args;
    va_start(args, txt);
    vprintf(txt, args);
    va_end(args);
    printf("\n");
}

void LOG_ERROR(const char* txt, ...) {
    printf("ERROR: ");
    va_list args;
    va_start(args, txt);
    vprintf(txt, args);
    va_end(args);
    printf("\n");
}