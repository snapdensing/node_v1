/*
 * sense_current.h
 *
 *  Created on: Jan 6, 2016
 *      Author: Snap
 */

//#ifndef SENSE_CURRENT_H_
//#define SENSE_CURRENT_H_



//#endif /* SENSE_CURRENT_H_ */

#include <msp430.h>
#include "stdint.h"

void ConfigureADC(void);
uint16_t ADC_single_meas(int channel);
unsigned int absolute_val (int value);

// This is the main function to call for current measurement
//unsigned int *current_measure(int *pointer) {
void current_measure(char *pointer) {

	//unsigned int array[11];
	//unsigned int *pointer = array;
	unsigned int prev_val1, prev_val2, prev_val3;
	unsigned int curr_val1, curr_val2, curr_val3;
	unsigned int processing1, processing2, processing3;
	unsigned int period1, period2, period3;
	unsigned int crossing1, crossing2, crossing3;
	unsigned int sample_count;
	unsigned int diff12, diff23;
	unsigned int min1, max1, min2, max2, min3, max3;
	unsigned int midpoint;

	// Configure ADC by disabling the watchdog timer and
	// enabling all necessary ADC registers
	ConfigureADC();

	// Initialization of variables. I do think variables are initially set to 0
	// but anyway, I'm initializing them anyway since they are critical in the
	// code operation.

	prev_val1 = 1023; //start with max so you won't enter the loop
	prev_val2 = 1023;
	prev_val3 = 1023;
	processing1 = 0;
	processing2 = 0;
	processing3 = 0;
	period1 = 0;
	period2 = 0;
	period3 = 0;
	sample_count = 0;

	// Perform initial measurement for reference
	midpoint = ADC_single_meas(0); // Yes, I can actually read the midpoint (DC signal)
	curr_val1 = ADC_single_meas(1);
	curr_val2 = ADC_single_meas(2);
	curr_val3 = ADC_single_meas(3);

	// Initialize the minimum and maximum values to the present value
	min1 = curr_val1;
	max1 = curr_val1;
	min2 = curr_val2;
	max2 = curr_val2;
	min3 = curr_val3;
	max3 = curr_val3;

	ADC10AE0 = BIT1 + BIT3 + BIT5;

	// Game, the actual current measurement loop
	// While we are still in the middle of the operation and the period is not yet calculated...
	while(processing1 || processing2 || processing3 || (period1 == 0) || (period2 == 0) || (period3 == 0)){

		curr_val1 = ADC_single_meas(1);
		curr_val2 = ADC_single_meas(2);
		curr_val3 = ADC_single_meas(3);
		//samples[0][sample_count] = curr_val1;

		// detect the rising cycle of the sinusoid passing through the midpoint
		if ((prev_val1 < midpoint) && (curr_val1 >= midpoint)){

			processing1 = ~processing1; //toggle
			period1 = sample_count - period1; // get the delta to get the period
			crossing1 = sample_count;

		}

		// push minimum and maximum values depending on the current data
		if (min1 > curr_val1) {
			min1 = curr_val1;
		}

		if (max1 < curr_val1) {
			max1 = curr_val1;
		}


		// update previous value, in preparation for the next iteration
		prev_val1 = curr_val1;

		// and then just do the whole thing again, 2 more times, for the 2 other phases.

		//samples[1][sample_count] = curr_val2;

		if ((prev_val2 < midpoint) && (curr_val2 >= midpoint)){

			processing2 = ~processing2; //toggle
			period2 = sample_count - period2; // get the delta to get the period
			crossing2 = sample_count;

		}

		if (min2 > curr_val2) {
			min2 = curr_val2;
		}

		if (max2 < curr_val2) {
			max2 = curr_val2;
		}

		prev_val2 = curr_val2; // update previous value, in preparation for the next iteration

		//samples[2][sample_count] = curr_val3;

		if ((prev_val3 < midpoint) && (curr_val3 >= midpoint)){

			processing3 = ~processing3; //toggle
			period3 = sample_count - period3; // get the delta to get the period
			crossing3 = sample_count;

		}

		if (min3 > curr_val3) {
			min3 = curr_val3;
		}

		if (max3 < curr_val3) {
			max3 = curr_val3;
		}

		prev_val3 = curr_val3; // update previous value, in preparation for the next iteration

		sample_count++; // this acts as the time

	} //end of while
	// after the loop, the minimum and maximum values are already acquired
	// moreover, the period for all three phases have also been measured

	// it's time to measure the phase differences
	diff12 = absolute_val(crossing1 - crossing2); // abs in case mauna si channel 1 kay 2
	diff23 = absolute_val(crossing2 - crossing3); // abs in case mauna si channel 2 kay 3

	// Turn off ADC, since I have left it on during the whole current measurement loop.
	ADC10CTL0 &= ~ADC10ON;
	ADC10AE0 = 0;
	//while(1);	// remove this later, it's just here for debugging purposes.

	/*array[0] = min1;
	array[1] = max1;
	array[2] = period1;
	array[3] = min2;
	array[4] = max2;
	array[5] = period2;
	array[6] = min2;
	array[7] = max2;
	array[8] = period2;
	array[9] = diff12;
	array[10] = diff23;*/

	/*pointer[0] = min1;
	pointer[1] = max1;
	pointer[2] = period1;
	pointer[3] = min2;
	pointer[4] = max2;
	pointer[5] = period2;
	pointer[6] = min3;
	pointer[7] = max3;
	pointer[8] = period3;
	pointer[9] = diff12;
	pointer[10] = diff23;*/

	pointer[0] = (char)(min1>>8);
	pointer[1] = (char)(min1 & 0x00FF);
	pointer[2] = (char)(max1>>8);
	pointer[3] = (char)(max1 & 0x00FF);
	//pointer[4] = (char)(period1>>8);
	pointer[4] = (char)(period1 & 0x00FF);
	pointer[5] = (char)(min2>>8);
	pointer[6] = (char)(min2 & 0x00FF);
	pointer[7] = (char)(max2>>8);
	pointer[8] = (char)(max2 & 0x00FF);
	//pointer[10] = (char)(period2>>8);
	pointer[9] = (char)(period2 & 0x00FF);
	pointer[10] = (char)(min3>>8);
	pointer[11] = (char)(min3 & 0x00FF);
	pointer[12] = (char)(max3>>8);
	pointer[13] = (char)(max3 & 0x00FF);
	//pointer[16] = (char)(period3>>8);
	pointer[14] = (char)(period3 & 0x00FF);
	//pointer[18] = (char)(diff12>>8);
	pointer[15] = (char)(diff12 & 0x00FF);
	//pointer[20] = (char)(diff23>>8);
	pointer[16] = (char)(diff23 & 0x00FF);
	//return pointer;	// the following 11 values must be returned
				// min1, max1 (the min and max values for the current, phase 1)
				// min2, max2 (the min and max values for the current, phase 2)
				// min3, max3 (the min and max values for the current, phase 3)
				// period1, period2, period 3 (the periods for the 3 current phases)
				// diff1 and diff2 (the phase differences between the 3 phases)

}

void ConfigureADC(void) {

	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	ADC10CTL0 = SREF_0 + ADC10SHT_0 + ADC10ON + REFON + REFOUT;
		// VCC as ref,
		// SHT to 4 ccs
		// Internal 1.5V ref voltage on
		// 1.5V VREF to P1.4 (VREF+ pin)
	__delay_cycles(100); // wait 100us for reference voltage to settle
	return;
}

uint16_t ADC_single_meas(int channel) {

//	volatile uint16_t adc_holder;

	switch(channel) {
		case 0:
			ADC10CTL1 = INCH_8; // Connect Vref output to ADC to get midpoint
			break;
		case 1:
			ADC10CTL1 = INCH_1; // Channel A1
			break;
		case 2:
			ADC10CTL1 = INCH_3; // Channel A3
			break;
		case 3:
			ADC10CTL1 = INCH_5; // Channel A5
			break;
	}

	ADC10CTL0 |= (ENC + ADC10SC);
	while (ADC10CTL1 & BUSY);

	ADC10CTL0 &= ~ENC;	 // Disable the ADC

	return ADC10MEM;
}

// absolute value function for the phase difference calculation.
unsigned int absolute_val (int value) {
	return value >= 0 ? value : 0 - value;
}
