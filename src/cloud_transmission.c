#include <pthread.h>
#include "smart_home_energy_monitor.h"
#include "logger.h"

int enqueue(struct sensor_data sampled_data, unsigned int pos)
{
	pthread_mutex_lock(&transmission_queue_lock);

	strcpy(data_transmission_queue[pos].device_name, sampled_data.device_name);
	strcpy(data_transmission_queue[pos].timestamp, sampled_data.timestamp);
	data_transmission_queue[pos].power = sampled_data.power;

	pthread_mutex_unlock(&transmission_queue_lock);

	return 0;
}

int dequeue()
{
	pthread_mutex_lock(&transmission_queue_lock);

	if (batch_size == 0 || sizeof(data_transmission_queue) == 0) {
		pthread_mutex_unlock(&transmission_queue_lock);
		return -1; // Queue is empty
	}

	memset(data_transmission_queue, 0, sizeof(data_transmission_queue));
	batch_size = 0;

	pthread_mutex_unlock(&transmission_queue_lock);

	return 0;
}

int write_data_to_cloud()
{
	// In a real implementation, this would use MQTT, HTTP, etc.
	// For simulation, just write to a file

	#if 0
		// Simulate network issues (10% chance of failure)
		if (random_double(0.0, 1.0) < 0.1) {
		return false;
		}
	#endif

	unsigned int count = batch_size * APPLIANCE_COUNT;

	LOG_MSG("Writing data to cloud with batch size : %u", batch_size);

	FILE *file = fopen("cloud_transmission.json", "a");
	if (file == NULL) {
		LOG_MSG("Failed to open Cloud JSON transmission file");
		return -1;
	}

	fprintf(file, "{\n  \"batch\": [\n");

	for (unsigned int i = 0; i < count; i++) {
		fprintf(file, "    {\"device_id\": \"%s\", \"timestamp\": \"%s\", \"power_watts\": %.2f}",
			data_transmission_queue[i].device_name, data_transmission_queue[i].timestamp,
			data_transmission_queue[i].power);

		if (i < count - 1) {
			fprintf(file, ",");
		}
		fprintf(file, "\n");
	}

	fprintf(file, "  ],\n");
	fprintf(file, "  \"transmission_time\": \"%ld\"\n", time(NULL));
	fprintf(file, "}\n");

	fclose(file);

	LOG_MSG("Data Transmitted to cloud");

	return 0;
}

void *transmit_to_cloud()
{
	int ret = 0;

	while (1) {
		if (batch_size >= MIN_BATCH_SIZE) {
			ret = write_data_to_cloud();
			if (ret < 0) {
				LOG_MSG("Write to Cloud failed due to Network failure");
			}
			ret = dequeue();
			if (ret < 0) {
				LOG_MSG("Dequeue of cloud transmission data failed. Queue Empty");
			}
		} else {
			sleep(CLOUD_TRANSMISSION_INTERVAL);
		}
	}
}
