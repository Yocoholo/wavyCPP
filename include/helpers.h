#pragma once

enum LogLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
};

// Runtime log level — reads LOG_LEVEL env var at startup
// Usage: LOG_LEVEL=1 ./wavy.exe  (0=none, 1=info, 2=warning, 3=error)
extern LogLevel g_logLevel;
void init_log_level();

void debug(const char* fmt, ...);
void debug_info(const char* fmt, ...);
void debug_warning(const char* fmt, ...);
void debug_error(const char* fmt, ...);