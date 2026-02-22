#include "helpers.h"
#include <cstdarg>
#include <cstdlib>
#include <stdio.h>
#include <iostream>

LogLevel g_logLevel = LOG_LEVEL_NONE;

void init_log_level() {
    const char* env = std::getenv("LOG_LEVEL");
    if (env) {
        int val = std::atoi(env);
        if (val >= LOG_LEVEL_NONE && val <= LOG_LEVEL_ERROR)
            g_logLevel = static_cast<LogLevel>(val);
    }
}

void debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    std::cout << std::flush;
    va_end(args);
}

void debug_info(const char* fmt, ...) {
    if (g_logLevel < LOG_LEVEL_INFO) return;
    va_list args;
    va_start(args, fmt);
    printf("\033[36m[INFO] ");
    vprintf(fmt, args);
    printf("\033[0m\n");
    va_end(args);
}

void debug_warning(const char* fmt, ...) {
    if (g_logLevel < LOG_LEVEL_WARNING) return;
    va_list args;
    va_start(args, fmt);
    printf("\033[33m[WARNING] ");
    vprintf(fmt, args);
    printf("\033[0m\n");
    va_end(args);
}

void debug_error(const char* fmt, ...) {
    if (g_logLevel < LOG_LEVEL_ERROR) return;
    va_list args;
    va_start(args, fmt);
    printf("\033[31m[ERROR] ");
    vprintf(fmt, args);
    printf("\033[0m\n");
    va_end(args);
}