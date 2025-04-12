#include "smart_home_energy_monitor.h"
#include "logger.h"
#include <stdarg.h>

struct smart_home_logger logger = {NULL, PTHREAD_MUTEX_INITIALIZER, 0};

int log_init()
{
	if (pthread_mutex_init(&logger.log_mutex, NULL) != 0) {
		return -1;
	}

	pthread_mutex_lock(&logger.log_mutex);

	char time_str[32];

	if (logger.log_file != NULL) {
		fclose(logger.log_file);
	}

	logger.log_file = fopen(LOGGER_FILE, "a");
	if (logger.log_file == NULL) {
		pthread_mutex_unlock(&logger.log_mutex);
		pthread_mutex_destroy(&logger.log_mutex);
		return -1;
	}

	logger.initialized = 1;

	get_current_timestamp(time_str, sizeof(time_str));
	fprintf(logger.log_file, "[%s] Smart Home Energy Monitoring System \n", time_str);
	fprintf(logger.log_file, "[%s] Logger initialized \n", time_str);
	fflush(logger.log_file);

	pthread_mutex_unlock(&logger.log_mutex);

	return 0;
}

void log_message(const char *format, ...)
{
	if (!logger.initialized || logger.log_file == NULL) return;

	pthread_mutex_lock(&logger.log_mutex);

	if (logger.log_file == NULL) {
		pthread_mutex_unlock(&logger.log_mutex);
		return;
	}

	char time_str[32];

	get_current_timestamp(time_str, sizeof(time_str));

	fprintf(logger.log_file, "[%s] ", time_str);

	va_list args;
	va_start(args, format);
	vfprintf(logger.log_file, format, args);
	va_end(args);

	fprintf(logger.log_file, "\n");

	fflush(logger.log_file);

	pthread_mutex_unlock(&logger.log_mutex);
}


void log_close()
{
	pthread_mutex_lock(&logger.log_mutex);

	char time_str[32];

	if (logger.log_file != NULL) {
		get_current_timestamp(time_str, sizeof(time_str));
		fprintf(logger.log_file, "[%s] Logger closed\n", time_str);
		fflush(logger.log_file);
		fclose(logger.log_file);
		logger.log_file = NULL;
	}

	logger.initialized = 0;

	pthread_mutex_unlock(&logger.log_mutex);
	pthread_mutex_destroy(&logger.log_mutex);
}
