#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define APPLIANCE_COUNT		3

#define SQL_DB	"energy_monitor.db"

sqlite3 *db;

time_t last_aggregate;

char start_timestamp[32];

int store_five_minute_aggregate(const char *device_id, const struct tm *tm_interval);

void get_current_timestamp(char *buffer, size_t buffer_size);

int initialize_db();

int store_reading(const char *device_id, double power_reading);

struct sensor_data{
	char device_name[50];
	float power;
};

void *sample_sensor_data();
