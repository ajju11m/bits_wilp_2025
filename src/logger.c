#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "logger.h"
#include "smart_home_energy_monitor.h"

struct smartHome_logger logger;

// Initialize logger
int log_init()
{
	pthread_mutex_lock(&logger.log_mutex);

	char time_str[32];

	if (logger.log_file != NULL) {
		fclose(logger.log_file);
	}

	logger.log_file = fopen(LOGGER_FILE, "a");
	if (logger.log_file == NULL) {
		pthread_mutex_unlock(&logger.log_mutex);
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

// Log a message
void log_message(const char *format, ...)
{
	pthread_mutex_lock(&logger.log_mutex);

	char time_str[32];
	get_current_timestamp(time_str, sizeof(time_str));

	fprintf(logger.log_file, "[%s] ", time_str);

	// Print formatted message
	va_list args;
	va_start(args, format);
	vfprintf(logger.log_file, format, args);
	va_end(args);

	// End with newline
	fprintf(logger.log_file, "\n");

	// Flush to ensure immediate write to disk
	fflush(logger.log_file);

	pthread_mutex_unlock(&logger.log_mutex);
}


// Close logger
void log_close()
{
	char time_str[32];
	pthread_mutex_lock(&logger.log_mutex);

	if (logger.log_file != NULL) {
		get_current_timestamp(time_str, sizeof(time_str));
		fprintf(logger.log_file, "[%s] Logger closed\n", time_str);
		fflush(logger.log_file);
		fclose(logger.log_file);
		logger.log_file = NULL;
	}

	logger.initialized = 0;
	pthread_mutex_unlock(&logger.log_mutex);
}
