#include "smart_home_energy_monitor.h"
#include "logger.h"

float generate_random_float(float min, float max) {
	return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

const char* appliances[] = {"TV", "Fridge", "WashingMachine"};

struct sensor_data appliance_data;

void *sample_sensor_data()
{
	while (1) {
		for (int i = 0; i < APPLIANCE_COUNT; i++) {
			strcpy(appliance_data.device_name, appliances[i]);
			appliance_data.power = generate_random_float(10.0, 500.0);
			LOG_MSG("Sampled Data: Appliance - %s Power - %.2f W", appliance_data.device_name, appliance_data.power);
			store_reading(appliance_data.device_name, appliance_data.power);
		}

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
		sleep(5);  // wait 5 seconds before next reading
	}
}
