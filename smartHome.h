#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

void get_current_timestamp(char *buffer, size_t buffer_size);

sqlite3 *db;

int initialize_db();

int store_reading(const char *device_id, double power_reading);

struct sensor_data{
	char device_name[50];
	int power;
};

void *sample_sensor_data();

int log_init();

void log_message(const char *format, ...);

#define LOG_MSG(fmt, ...) \
    log_message(fmt, ##__VA_ARGS__)

void log_close();
