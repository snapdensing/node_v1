/*
 * sense_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"
#include "sense_current.h"

/* Function Prototypes */
int tempSense();
int senseDHT(unsigned char *p);
unsigned int Battery_supply_nonreg(void);

/* Sensor data packet assembly
 * Returns length of data packet
 */

#ifdef SENSOR_BATT
int buildSense(char *txbuf, unsigned int sensor_flag, unsigned int tx_count, unsigned int *batt_out){
//int buildSense(char *tx_data, unsigned int sensor_flag, unsigned int tx_count, unsigned int *batt_out){
#else
int buildSense(char *txbuf, unsigned int sensor_flag, unsigned int tx_count){
//int buildSense(char *tx_data, unsigned int sensor_flag, unsigned int tx_count){
#endif

	int temp_int;
	int batt;
	int dht11_rh,dht11_temp;
	char dht22_rh[2];
	char dht22_temp[2];
	char current[17];

	/* Temperature (Internal) */
#ifdef SENSOR_TEMPINT
	temp_int = tempSense();
#endif

	/* Unregulated battery voltage */
#ifdef SENSOR_BATT
	batt = Battery_supply_nonreg();
#endif

	/* DHT-11 */
#ifdef SENSOR_DHT11
	unsigned char dht[6];
	int err;

	while(TA1CCTL0 & CCIE);
	err = 0;
	err = senseDHT(dht);                        // Read DHT11
	if(err) {                                   // If error...
		dht11_temp = -1;
		dht11_rh = -1;
	}
	else {                                      // No error...
		dht11_temp = dht[3];
		dht11_rh = dht[1];
	}
#endif

	/* DHT-22 */
#ifdef SENSOR_DHT22
	unsigned char dht[6];
	int err;

	while(TA1CCTL0 & CCIE);
	err = 0;
	err = senseDHT(dht);                        // Read DHT11
	if(err) {                                   // If error...
		dht22_temp[0] = 0xff;
		dht22_temp[1] = 0xff;
		dht22_rh[0] = 0xff;
		dht22_rh[1] = 0xff;
	}
	else {                                      // No error...
		dht22_temp[0] = dht[3];
		dht22_temp[1] = dht[4];
		dht22_rh[0] = dht[1];
		dht22_rh[1] = dht[2];
	}
#endif

#ifdef SENSOR_CURRENT

	current_measure(current);
#endif

	/* Payload Assembly */
	int i=16;
	int j;
	txbuf[14] = (char)'D';
	txbuf[15] = (char)tx_count;

	/** Unregulated Battery Voltage **/
	if (sensor_flag & 1){
		txbuf[i++] = 0x00;
		txbuf[i++] = (char)(batt >> 8);
		txbuf[i++] = (char)(batt & 0x00ff); //replace with battery voltage
	}

	/** Internal Temperature **/
	if (sensor_flag & 2){
		txbuf[i++] = 0x01;
		txbuf[i++] = (char)(temp_int >> 8);
		txbuf[i++] = (char)(temp_int & 0x00ff);
	}

	/** DHT 11 Temperature **/
	if (sensor_flag & 4){
		txbuf[i++] = 0x02;
		txbuf[i++] = 0x00; //zero-padded
		txbuf[i++] = (char)dht11_temp;
	}

	/** DHT 11 Humidity **/
	if (sensor_flag & 8){
		txbuf[i++] = 0x03;
		txbuf[i++] = 0x00; //zero-padded
		txbuf[i++] = (char)dht11_rh;
	}

	/** DHT 22 Temperature **/
	if (sensor_flag & 16){
		txbuf[i++] = 0x04;
		txbuf[i++] = dht22_temp[0];
		txbuf[i++] = dht22_temp[1];
	}

	/** DHT 22 Humidity **/
	if (sensor_flag & 32){
		txbuf[i++] = 0x05;
		txbuf[i++] = dht22_rh[0];
		txbuf[i++] = dht22_rh[1];
	}

	/** Current sensor **/
	if (sensor_flag & 64){
		txbuf[i++] = 0x06;
		for (j=0; j<17; j++)
			txbuf[i++] = current[j];
	}

	*batt_out = batt;

	return i; //return tx_data length
}

/* Unregulated battery voltage (Joy) */
#ifdef SENSOR_BATT
unsigned int Battery_supply_nonreg(void)
{
    unsigned int value=0;
    ADC10CTL1 = INCH_2 + ADC10SSEL_2;
    ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + REFON + REF2_5V;
    ADC10AE0 |= BIT2; //A2 ADC input
    __delay_cycles(1000);
    ADC10CTL0 |= ENC + ADC10SC;
    while(ADC10CTL1 & ADC10BUSY);
    value = ADC10MEM;
    ADC10CTL0 &= ~(ENC + ADC10ON);
    return value;
}
#endif

/* Internal Temperature sensing (Joy) */
#ifdef SENSOR_TEMPINT
int tempSense() {
	int raw_temp=0;
	int done=0;

	//Initialize ADC
	ADC10CTL0=SREF_1 + REFON + ADC10ON + ADC10SHT_3;		//1.5V ref, internal reference on, 64 clock cycles/sample
	ADC10CTL1=INCH_10+ ADC10DIV_3 + ADC10SSEL_2;			//temp sensor channel, clock/4, MCLK source

	while (done==0) {
		__delay_cycles(1000);							//wait for reference to settle
		ADC10CTL0 |= ENC + ADC10SC;						//enable conversion and start conversion
		while(ADC10CTL1 & BUSY);						//converting...
		raw_temp=ADC10MEM;								//store ADC output in variable
		ADC10CTL0&=~ENC;                 				//disable ADC
		done=1;
	}
	return(int) raw_temp;								//pass to main
}
#endif

/* DHT-11 Humidity sensing (Grace) */
#ifdef SENSOR_DHT11
int senseDHT(unsigned char *p)
{
                                                                // Note: Timer1 must be continuous mode (MC_2) at 1 MHz
    const unsigned b = BIT1;                                    // I/O bit
    const unsigned char *end = p + 6;                           // End of data buffer
    register unsigned char m = 1;                               // First byte will have only start bit
    register unsigned st, et;                                   // Start and end times

    // Initialize DCO, TimerA and data port
    DCOCTL = 0;                                     // Run at 1 MHz
    BCSCTL1 = CALBC1_1MHZ;                          	//
    DCOCTL  = CALDCO_1MHZ;                          	//
    TA1CCTL0 = OUT;                                    	// Setup serial tx I/O
    P2SEL =  1;                                      // This config is SMCLK
    P2SEL2 = 0;
    P2DIR = BIT1;                                   // Set P2.1 to output
    TA1CTL = TASSEL_2 | MC_2;                        	// Timer1 SMCLK, continuous mode (timer counts to 0FFFFh)


    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;                // Clear data buffer
                                                                //
    P2OUT &= ~b;                                                // Pull low
    P2DIR |= b;                                                 // Output
    P2REN &= ~b;                                                // Drive low
    st = TA1R; while((TA1R - st) < 18000);                        // Wait 18 ms
    P2REN |= b;                                                 // Pull low
    P2OUT |= b;                                                 // Pull high
    P2DIR &= ~b;                                                // Input
                                                                //
    st = TA1R;                                                   // Get start time for timeout
    while(P2IN & b) if((TA1R - st) > 100) return -1;             // Wait while high, return if no response
    et = TA1R;                                                   // Get start time for timeout
    do {                                                        //
        st = et;                                                // Start time of this bit is end time of previous bit
        while(!(P2IN & b)) if((TA1R - st) > 100) return -2;      // Wait while low, return if stuck low
        while(P2IN & b) if((TA1R - st) > 200) return -3;         // Wait while high, return if stuck high
        et = TA1R;                                               // Get end time
        if((et - st) > 110) *p |= m;                            // If time > 110 us, then it is a one bit
        if(!(m >>= 1)) m = 0x80, ++p;                           // Shift mask, move to next byte when mask is zero
    } while(p < end);                                           // Do until array is full
                                                                //
    p -= 6;                                                     // Point to start of buffer
    if(p[0] != 1) return -4;                                    // No start bit
    if(((p[1] + p[2] + p[3] + p[4]) & 0xFF) != p[5]) return -5; // Bad checksum
                                                                //
    return 0;                                                   // Good read
}
#endif

/* DHT-22 Humidity sensing (Grace) */
#ifdef SENSOR_DHT22
int senseDHT(unsigned char *p)
{
                                                                // Note: Timer1 must be continuous mode (MC_2) at 1 MHz
    const unsigned b = BIT1;                                    // I/O bit
    const unsigned char *end = p + 6;                           // End of data buffer
    register unsigned char m = 1;                               // First byte will have only start bit
    register unsigned st, et;                                   // Start and end times

    // Initialize DCO, TimerA and data port
    DCOCTL = 0;                                     // Run at 1 MHz
    BCSCTL1 = CALBC1_1MHZ;                          	//
    DCOCTL  = CALDCO_1MHZ;                          	//
    TA1CCTL0 = OUT;                                    	// Setup serial tx I/O
    P2SEL =  1;                                      // This config is SMCLK
    P2SEL2 = 0;
    P2DIR = BIT1;                                   // Set P2.1 to output
    TA1CTL = TASSEL_2 | MC_2;                        	// Timer1 SMCLK, continuous mode (timer counts to 0FFFFh)


    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;                // Clear data buffer
                                                                //
    P2OUT &= ~b;                                                // Pull low
    P2DIR |= b;                                                 // Output
    P2REN &= ~b;                                                // Drive low
    st = TA1R; while((TA1R - st) < 18000);                        // Wait 18 ms
    P2REN |= b;                                                 // Pull low
    P2OUT |= b;                                                 // Pull high
    P2DIR &= ~b;                                                // Input
                                                                //
    st = TA1R;                                                   // Get start time for timeout
    while(P2IN & b) if((TA1R - st) > 100) return -1;             // Wait while high, return if no response
    et = TA1R;                                                   // Get start time for timeout
    do {                                                        //
        st = et;                                                // Start time of this bit is end time of previous bit
        while(!(P2IN & b)) if((TA1R - st) > 100) return -2;      // Wait while low, return if stuck low
        while(P2IN & b) if((TA1R - st) > 200) return -3;         // Wait while high, return if stuck high
        et = TA1R;                                               // Get end time
        if((et - st) > 110) *p |= m;                            // If time > 110 us, then it is a one bit
        if(!(m >>= 1)) m = 0x80, ++p;                           // Shift mask, move to next byte when mask is zero
    } while(p < end);                                           // Do until array is full
                                                                //
    p -= 6;                                                     // Point to start of buffer
    if(p[0] != 1) return -4;                                    // No start bit
    if(((p[1] + p[2] + p[3] + p[4]) & 0xFF) != p[5]) return -5; // Bad checksum
                                                                //
    return 0;                                                   // Good read
}
#endif
