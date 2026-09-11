#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <stdio.h>

typedef enum LogLevel {
	LOG_LEVEL_DEBUG = 0,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL,
	LOG_LEVEL_COUNT
} LogLevel;

void log_init(LogLevel level, FILE *sink);
void log_shutdown(void);
void log_set_level(LogLevel level);
LogLevel log_get_level(void);
const char *log_level_name(LogLevel level);
int log_enabled(LogLevel level);
unsigned log_message_count(LogLevel level);
void log_write(LogLevel level, const char *file, int line, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));
void log_flush(void);
void log_summary(void);

#define LOG_DEBUG(...) log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_write(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_write(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif /* UTIL_LOG_H */
