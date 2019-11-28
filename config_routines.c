/*
 * config_routines.c
 *
 *  Created on: Jul 1, 2015
 *      Author: Snap
 */

#include "defines.h"

/* Checks presence of all sensors and sets an integer flag
 * Flag fields (1 if present):
 * -> bit 0: Battery voltage
 * -> bit 1: Temperature (internal)
 * -> bit 2: DHT-11 Temperature (external)
 * -> bit 3: DHT-11 Humidity
 * -> bit 4: DHT-22 Temperature
 * -> bit 5: DHT-22 Humidity
 */
//unsigned int detect_sensor(void){
void detect_sensor(unsigned int *sensor_flagp){
	//unsigned int sensor;

	*sensor_flagp = 0;

	// Battery voltage
#ifdef SENSOR_BATT
	*sensor_flagp += 1;
#endif

	// Temperature (internal)
#ifdef SENSOR_TEMPINT
	*sensor_flagp += 2;
#endif

	// DHT-11 Temperature & Humidity (external)
#ifdef SENSOR_DHT11
	*sensor_flagp += 12;
#endif

	// DHT-22 Temperature & Humidity (external)
#ifdef SENSOR_DHT22
	*sensor_flagp += 48;
#endif

	// Current
#ifdef SENSOR_CURRENT
	*sensor_flagp += 64;
#endif

}
