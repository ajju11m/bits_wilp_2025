#include "smart_home_energy_monitor.h"
#include "logger.h"

float generate_random_float(float min, float max) {
	return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

const char* appliances[] = {"TV", "Fridge", "WashingMachine"};

struct sensor_data appliance_data;

void *sample_sensor_data()
{
	int ret = 0;
	srand(time(NULL));
	memset(data_transmission_queue, 0, sizeof(data_transmission_queue));
	while (1) {
		for (int i = 0; i < APPLIANCE_COUNT; i++) {
			strcpy(appliance_data.device_name, appliances[i]);
			appliance_data.power = generate_random_float(10.0, 500.0);
			get_current_timestamp(appliance_data.timestamp, sizeof(appliance_data.timestamp));
			LOG_MSG("Sampled Data: Appliance - %s Power - %.2f W", appliance_data.device_name, appliance_data.power);
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

		time_t now = time(NULL);
		struct tm *tm_now = localtime(&now);

		if (last_aggregate == 0 || ((now - last_aggregate) >= 300)) {
			// Create a temporary tm structure to hold the rounded time
			char hour_timestamp[20];
			strftime(hour_timestamp, sizeof(hour_timestamp), "%Y-%m-%dT%H-%M-%S", tm_now);

			// Create aggregates for each sensor
			for (int i = 0; i < APPLIANCE_COUNT; i++) {
				// Renamed function to reflect it's now a 10-minute aggregate
				strcpy(appliance_data.device_name, appliances[i]);
				store_five_minute_aggregate(appliance_data.device_name, tm_now);
			}
			last_aggregate = now;
		}
		sleep(SENSOR_SAMPLING_INTERVAL);  // wait before next reading
	}
}
