#include "util/log.h"

#include <stdarg.h>
#include <string.h>
#include <time.h>

typedef struct LogState {
	LogLevel level;
	FILE *sink;
	unsigned counts[LOG_LEVEL_COUNT];
	int initialized;
} LogState;

static LogState g_log = { LOG_LEVEL_INFO, NULL, { 0, 0, 0, 0, 0 }, 0 };

static const char *const g_level_names[LOG_LEVEL_COUNT] = {
	"DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static FILE *log_sink(void)
{
	return g_log.sink ? g_log.sink : stderr;
}

static const char *log_basename(const char *path)
{
	const char *slash = strrchr(path, '/');
	return slash ? slash + 1 : path;
}

static void log_timestamp(char *buf, size_t size)
{
	time_t now = time(NULL);
	struct tm tm_buf;
	if (localtime_r(&now, &tm_buf) == NULL) {
		snprintf(buf, size, "--:--:--");
		return;
	}
	strftime(buf, size, "%H:%M:%S", &tm_buf);
}

static void log_count(LogLevel level)
{
	if (level < LOG_LEVEL_COUNT)
		g_log.counts[level]++;
}

void log_init(LogLevel level, FILE *sink)
{
	memset(g_log.counts, 0, sizeof(g_log.counts));
	g_log.level = level;
	g_log.sink = sink;
	g_log.initialized = 1;
	LOG_DEBUG("logging initialized at level %s", log_level_name(level));
}

void log_shutdown(void)
{
	log_flush();
	g_log.initialized = 0;
	g_log.sink = NULL;
}

void log_set_level(LogLevel level)
{
	if (level < LOG_LEVEL_COUNT)
		g_log.level = level;
}

LogLevel log_get_level(void)
{
	return g_log.level;
}

const char *log_level_name(LogLevel level)
{
	if (level < LOG_LEVEL_COUNT)
		return g_level_names[level];
	return "?????";
}

int log_enabled(LogLevel level)
{
	return level >= g_log.level;
}

unsigned log_message_count(LogLevel level)
{
	return level < LOG_LEVEL_COUNT ? g_log.counts[level] : 0u;
}

void log_write(LogLevel level, const char *file, int line, const char *fmt, ...)
{
	char stamp[16];
	va_list args;

	log_count(level);
	if (!log_enabled(level))
		return;

	log_timestamp(stamp, sizeof(stamp));
	fprintf(log_sink(), "[%s] %-5s %s:%d: ", stamp, log_level_name(level),
		log_basename(file), line);

	va_start(args, fmt);
	vfprintf(log_sink(), fmt, args);
	va_end(args);

	fputc('\n', log_sink());
	if (level >= LOG_LEVEL_ERROR)
		log_flush();
}

void log_flush(void)
{
	fflush(log_sink());
}

void log_summary(void)
{
	int i;
	for (i = 0; i < LOG_LEVEL_COUNT; i++) {
		LogLevel level = (LogLevel)i;
		if (log_message_count(level) > 0)
			LOG_INFO("log summary: %-5s x%u", log_level_name(level),
				 log_message_count(level));
	}
}
