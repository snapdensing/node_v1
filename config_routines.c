/*
 * config_routines.c
 *
 *  Created on: Jul 1, 2015
 *      Author: Snap
 */

#include "defines.h"

/* Check if sensor present. Returns 1 if yes
 * Sensor IDs:
 * -> 0x00: Battery voltage
 * -> 0x01: Temperature (Internal)
 * -> 0x02: Temperature (External)
 * -> 0x03: Humidity
 */
int check_sensor(int sensor_id){
	int batt;
	int temp_int,dht11_temp;
	int dht11_hum;
	int dht22_temp,dht22_hum;
	int current;

#ifdef SENSOR_BATT
	batt = 1;
#else
	batt = 0;
#endif

#ifdef SENSOR_TEMPINT
	temp_int = 1;
#else
	temp_int = 0;
#endif

#ifdef SENSOR_DHT11
	dht11_temp = 1;
#else
	dht11_temp = 0;
#endif

#ifdef SENSOR_DHT11
	dht11_hum = 1;
#else
	dht11_hum = 0;
#endif

#ifdef SENSOR_DHT22
	dht22_temp = 1;
#else
	dht22_temp = 0;
#endif

#ifdef SENSOR_DHT22
	dht22_hum = 1;
#else
	dht22_hum = 0;
#endif

#ifdef SENSOR_CURRENT
	current = 1;
#else
	current = 0;
#endif

	switch(sensor_id){
	case 0x00:
		return batt;
	case 0x01:
		return temp_int;
	case 0x02:
		return dht11_temp;
	case 0x03:
		return dht11_hum;
	case 0x04:
		return dht22_temp;
	case 0x05:
		return dht22_hum;
	case 0x06:
		return current;
	default:
		return 0;
	}
}

/* Checks presence of all sensors and sets an integer flag
 * Flag fields (1 if present):
 * -> bit 0: Battery voltage
 * -> bit 1: Temperature (internal)
 * -> bit 2: DHT-11 Temperature (external)
 * -> bit 3: DHT-11 Humidity
 * -> bit 4: DHT-22 Temperature
 * -> bit 5: DHT-22 Humidity
 */
unsigned int detect_sensor(void){
	int sensor_flag = 0;

	// Battery voltage
	if (check_sensor(0x00)==1)
		sensor_flag += 1;

	// Temperature (internal)
	if (check_sensor(0x01)==1)
		sensor_flag += 2;

	// DHT-11 Temperature (external)
	if (check_sensor(0x02)==1)
		sensor_flag += 4;

	// DHT-11 Humidity
	if (check_sensor(0x03)==1)
		sensor_flag += 8;

	// DHT-22 Temperature (external)
	if (check_sensor(0x04)==1)
		sensor_flag += 16;

	// DHT-22 Humidity
	if (check_sensor(0x05)==1)
		sensor_flag += 32;

	// Current
	if (check_sensor(0x06)==1)
		sensor_flag += 64;

	return sensor_flag;
}
