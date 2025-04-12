#include <pthread.h>

// Logger configuration
struct smart_home_logger {
	FILE *log_file;
	pthread_mutex_t log_mutex;
	int initialized;
};

#define LOGGER_FILE "smart_home_monitor.log"

#define LOG_MSG(fmt, ...) \
    log_message(fmt, ##__VA_ARGS__)

int log_init();
void log_message(const char *format, ...);
void log_close();
