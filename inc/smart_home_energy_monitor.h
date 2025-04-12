#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define APPLIANCE_COUNT			3

#define SENSOR_SAMPLING_INTERVAL	5
#define CLOUD_TRANSMISSION_INTERVAL	60 /* Transmitting 1 minute of data for every device */
#define MIN_BATCH_SIZE			(CLOUD_TRANSMISSION_INTERVAL/SENSOR_SAMPLING_INTERVAL)

#define SQL_DB	"energy_monitor.db"

unsigned int batch_size;

sqlite3 *db;

time_t last_aggregate;

char start_timestamp[32];

int store_five_minute_aggregate(const char *device_id, const struct tm *tm_interval);

void get_current_timestamp(char *buffer, size_t buffer_size);

int initialize_db();

struct sensor_data{
	char device_name[50];
	char timestamp[64];
	int power;
};

int store_reading(struct sensor_data sampled_data);

pthread_mutex_t transmission_queue_lock;
struct sensor_data data_transmission_queue[64];

int enqueue(struct sensor_data sampled_data, unsigned int pos);

int write_data_to_cloud();

void *sample_sensor_data();
void *transmit_to_cloud();
