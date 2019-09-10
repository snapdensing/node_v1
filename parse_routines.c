/*
 * parse_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>

/* Parse AT Command Response
 * Currently supported AT command responses
 * - SH (Upper address)
 * - SL (Lower address)
 * - D6
 */
int parse_atres(char com0, char com1, char *returndata, char *packet, int length){
	int j;
	if (packet[3]==0x88){

		// Match received with parameter (ignore otherwise)
		if ((packet[5] == com0) && (packet[6] == com1)){

			// Parameter SH
			if ((com0 == 'S') && (com1 == 'H')){
				// Extract upper byte of address
				for (j=0; j<4; j++){
					returndata[j] = packet[j+8];
				}
				return 1;
			}

			// Parameter SL
			else if ((com0 == 'S') && (com1 == 'L')){
				// Extract lower byte of address
				for (j=0; j<4; j++){
					returndata[j+4] = packet[j+8];
				}
				return 1;
			}

			// Parameter D6
			else if ((com0 == 'D') && (com1 == '6')){
				return 1;
			}
		}
	}
	return 0;
}

// Parse Acknowledge signal from base station
int parse_ack(char *packet, int length, char *base_addr){
	int success = 0;
	int j;

	// Check frame type
	if (packet[3]==0x90){

		// Check data
		if ((packet[15] == 'A') && (length == 13)){
			success = 1;
			// Extract base address
			for (j=0; j<8; j++){
				base_addr[j] = packet[j+4];
			}
		}
	}

	return success;
}

// Parse (Sensing) start signal from base station
int parse_start(char *packet, int length, int *sample_period){
	int success = 0;

	// Check frame type
	if (packet[3]==0x90){

		// Check data
		if ((packet[15] == 'S') && (length == 14)){
			success = 1;
			*sample_period = (int) packet[16]; // 1-byte sample period
		}
	}

	return success;
}

/* Parse (Sensing) stop signal from base station
 * - Does not authenticate stop signal yet
 */
int parse_stop(char *packet, int length){
	int success = 0;

	// Check frame type
	if (packet[3]==0x90){

		// Check data
		if ((packet[15] == 'X') && (length == 13)){
			success = 1;
		}
	}

	return success;
}
