#include <pthread.h>
#include "smart_home_energy_monitor.h"
#include "logger.h"

int main() {
	pthread_t sensor_thread;

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

	// Create threads
	if (pthread_create(&sensor_thread, NULL, sample_sensor_data, NULL) != 0) {
		LOG_MSG("Failed to create sensor thread");
		return 1;
	}

	pthread_join(sensor_thread, NULL);
	log_close();
	return 0;
}
