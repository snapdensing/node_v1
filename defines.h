/*
 * defines.h
 *
 *  Created on: Mar 27, 2015
 *      Author: Snap
 */

/* Sensor Configuration */
//#define SENSOR_BATT
#define SENSOR_TEMPINT
//#define SENSOR_DHT11
//#define SENSOR_DHT22
//#define SENSOR_CURRENT

//#define SLEEP_UC
//#define SLEEP_XBEE

/* State encoding */

#define	S_RTS1		10
#define S_RTS2		11

#define S_ADDR1		0
#define S_ADDR2		1
#define S_ADDR3		2
#define S_ADDR4		3

#define S_INIT		4
#define S_READY		5
#define S_SENSE		6
#define S_TRACE1	7
#define S_TRACE2	8
#define S_WINDOW	9

/* Timer-relative periods */
/** Sensing Period **/
#define SAMPLE_PERIOD	10
/** "Present" Broadcast Period **/
#define PRES_PERIOD		2
/** Stop window Period **/
#define STOP_PERIOD		5

#define MAC_ACK_TIMEOUT	10
#define NODELIST_MAX	20
#define RXBUF_MAX		100
#define TRACE_TIMEOUT	100
