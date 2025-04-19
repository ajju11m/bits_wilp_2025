#include "smart_home_energy_monitor.h"
#include <signal.h>

// Signal handler for graceful shutdown
void handle_signal(int sig)
{
	if (sig == SIGINT) {
		LOG_MSG(" ====== Received shutdown signal. Stopping system gracefully ====== ");
		system_running = false;
	}
}

int main()
{
	pthread_t sensor_thread;
	pthread_t transmission_thread;
	system_running = true;

	printf("Smart Home Energy Monitoring System\n");
	printf("===================================\n\n");

	if (log_init()) {
		fprintf(stderr, "Failed to setup logger\n");
		return -1;
	}

	signal(SIGINT, handle_signal);

	if (initialize_db()) {
		LOG_MSG("Failed to initialize Database");
		return -1;
	}

	srand(time(NULL));

	last_aggregate = time(NULL);

	pthread_mutex_init(&transmission_queue_lock, NULL);

	strftime(start_timestamp, sizeof(start_timestamp), "%Y-%m-%dT%H:%M:%S",
		 localtime(&last_aggregate));

	// Create sensor sampling thread
	if (pthread_create(&sensor_thread, NULL, sample_sensor_data, NULL) != 0) {
		LOG_MSG("Failed to create sensor thread");
		return -1;
	}

	// Create Data Transmission to Cloud thread
	if (pthread_create(&transmission_thread, NULL, transmit_to_cloud, NULL) != 0) {
		LOG_MSG("Failed to create sensor thread");
		return -1;
	}

	pthread_join(sensor_thread, NULL);
	pthread_join(transmission_thread, NULL);
	pthread_mutex_destroy(&transmission_queue_lock);

	sqlite3_close(db);
	LOG_MSG("Database Closed");

	log_close();

	printf("\n===================================\n");
	printf("Energy Monitoring System shut down\n");

	return 0;
}
