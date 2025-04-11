#include <pthread.h>
#include "smart_home_energy_monitor.h"
#include "logger.h"

int main() {
	pthread_mutex_init(&transmission_queue_lock, NULL);
	pthread_t sensor_thread;
	pthread_t transmission_thread;

	printf("Smart Home Energy Monitoring System\n");
	printf("===================================\n\n");

	if (log_init()) {
		fprintf(stderr, "Failed to setup logger\n");
		return -1;
	}

	if (initialize_db()) {
		LOG_MSG("Failed to initialize Database");
		return -1;
	}

	srand(time(NULL));

	last_aggregate = time(NULL);

	strftime(start_timestamp, sizeof(start_timestamp), "%Y-%m-%dT%H:%M:%S",
		 localtime(&last_aggregate));

	// Create threads
	if (pthread_create(&sensor_thread, NULL, sample_sensor_data, NULL) != 0) {
		LOG_MSG("Failed to create sensor thread");
		return -1;
	}

	if (pthread_create(&transmission_thread, NULL, transmit_to_cloud, NULL) != 0) {
		LOG_MSG("Failed to create sensor thread");
		return -1;
	}

	pthread_join(sensor_thread, NULL);
	pthread_join(transmission_thread, NULL);
	pthread_mutex_destroy(&transmission_queue_lock);
	log_close();
	return 0;
}
