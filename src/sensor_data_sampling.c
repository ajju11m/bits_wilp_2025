#include "smart_home_energy_monitor.h"
#include "logger.h"

int generate_random(int min, int max) {
	return min + rand() % (max - min + 1);
}

const char* appliances[] = {"TV", "Fridge", "WashingMachine"};

struct sensor_data appliance_data;

/**
 * sample_sensor_data() : Continuously samples power consumption data from all appliances,
 * 			  stores readings in a database, queues them for cloud transmission,
 *			  and performs 5-minute data aggregation.
 *
 * This function is intended to be passed to pthread_create() and runs indefinitely
 * in a loop. For each appliance:
 * - It generates random power usage data.
 * - Timestamps the data.
 * - Stores it in the database.
 * - Queues the data for transmission.
 * Every 5 minutes (or on the first run), it also stores aggregated data.
 *
 * Return:  NULL on thread termination (although it runs indefinitely).
 */
void *sample_sensor_data()
{
	int ret = 0, i;
	time_t now;
	struct tm *tm_now;

	memset(data_transmission_queue, 0, sizeof(data_transmission_queue));
	while (system_running) {
		for (i = 0; i < APPLIANCE_COUNT; i++) {
			strcpy(appliance_data.device_name, appliances[i]);
			appliance_data.power = generate_random(10, 500);
			get_current_timestamp(appliance_data.timestamp, sizeof(appliance_data.timestamp));
			LOG_MSG("Sampled Data: Appliance - %s Power - %d W", appliance_data.device_name, appliance_data.power);
			ret = store_reading(appliance_data);
			if (ret < 0) {
				LOG_MSG("Data Store to DB failed");
			}
			ret = enqueue(appliance_data, ((batch_size * APPLIANCE_COUNT) + i));
			if (ret < 0) {
				LOG_MSG("Queuing of data for cloud transfer failed");
			}
		}
		batch_size++;

		now = time(NULL);
		tm_now = localtime(&now);

		if (last_aggregate == 0 || ((now - last_aggregate) >= 300)) {

			// Create aggregates for each sensor
			for (int j= 0; j < APPLIANCE_COUNT; j++) {
				strcpy(appliance_data.device_name, appliances[j]);
				store_five_minute_aggregate(appliance_data.device_name, tm_now);
			}
			last_aggregate = now;
		}
		sleep(SENSOR_SAMPLING_INTERVAL);  // wait before next reading
	}

	LOG_MSG("Sensor reading thread terminated");
	return NULL;
}
