#include "smart_home_energy_monitor.h"
#include "logger.h"

float generate_random_float(float min, float max) {
	return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

const char* appliances[] = {"TV", "Fridge", "WashingMachine"};

struct sensor_data appliance_data;

void *sample_sensor_data()
{
	srand(time(NULL));
	while (1) {
		for (int i = 0; i < APPLIANCE_COUNT; i++) {
			strcpy(appliance_data.device_name, appliances[i]);
			appliance_data.power = generate_random_float(10.0, 500.0);
			LOG_MSG("Sampled Data: Appliance - %s Power - %.2f W", appliance_data.device_name, appliance_data.power);
			store_reading(appliance_data.device_name, appliance_data.power);
		}
		sleep(5);  // wait 5 seconds before next reading
	}
}
