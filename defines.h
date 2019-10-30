/*
 * defines.h
 *
 *  Created on: Mar 27, 2015
 *      Author: Snap
 */

#define MODE_DEBUG
#define DEBUG_CHARGING

/* Sensor Configuration */
#define SENSOR_BATT
#define SENSOR_TEMPINT
//#define SENSOR_DHT11
#define SENSOR_DHT22
//#define SENSOR_CURRENT

//#define SLEEP_UC
//#define SLEEP_XBEE

/* Broadcast */
//#define BROADCAST

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
#define S_STOP		17

#define S_DEBUG		12
#define S_DBRD		13
#define S_DUNI		14
#define S_DPLRES	15
#define S_DCHRES	16
#define S_DQRES1	18
#define S_DQRES2    19
#define S_DQRES3    20


/* Next state assignment */
#define NS_SENSE	S_WINDOW
#define NS_WINLOOP	S_SENSE
#define NS_WINBRK	S_STOP
#define NS_STOP		S_DEBUG

#ifdef MODE_DEBUG
#define NS_DEBUG1	S_DBRD
#define NS_DEBUG2	S_DUNI
#define NS_DEBUG3	S_DEBUG
#define NS_DEBUG4	S_DPLRES
#define NS_DEBUG5	S_DCHRES
#define NS_DBRDLOOP	S_DBRD
#define NS_DBRDBRK	S_DEBUG
#define NS_DUNILOOP S_DUNI
#define NS_DUNIBRK	S_DEBUG
#define NS_DPLRES	S_DEBUG
#define NS_DCHRES	S_DEBUG
#endif

/* Timer-relative periods */
/** Sensing Period **/
#define SAMPLE_PERIOD	10
/** "Present" Broadcast Period **/
#define PRES_PERIOD		2
/** Stop window Period **/
#define STOP_PERIOD		5
/** Debug state battery sample period **/
#define STANDBY_SAMPLEBATT 1000

#define MAC_ACK_TIMEOUT	10
#define NODELIST_MAX	20
#define RXBUF_MAX		100
#define TRACE_TIMEOUT	100

#define PARAM_PL	0

/** Battery charging thresholds **/
#define BATT_VTHLO 0x02e0 // 3.6V
#define BATT_VTHHI 0x0346 // 4.1V
//#define BATT_VTHLO 0x02f0
//#define BATT_VTHHI 0x0310
