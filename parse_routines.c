/*
 * parse_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"

/* Parse AT Command Response
 * Currently supported AT command responses
 * - SH (Upper address)
 * - SL (Lower address)
 * - D6
 * - PL
 * - CH
 */
int parse_atres(char com0, char com1, char *returndata, char *packet, unsigned int length){
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

			// Parameter PL
			else if ((com0 == 'P') && (com1 == 'L')){
				// Extract PL parameter value
				returndata[0] = packet[7];
				//*returndata = packet[7];
				return 1;
			}

			// Parameter CH
			else if ((com0 == 'C') && (com1 == 'H')){
				// Extract CH parameter value
				returndata[0] = packet[7];
				return 1;
			}

			// Parameter ID
			else if ((com0 == 'I') && (com1 == 'D')){
			    return 1;
			}
		}
	}
	return 0;
}

// Parse Acknowledge signal from base station
int parse_ack(char *packet, unsigned int length, char *base_addr){
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
int parse_start(char *packet, unsigned int length, unsigned int *sample_period){
	int success = 0;

	// Check frame type
	if (packet[3]==0x90){

		// Check data
		if ((packet[15] == 'S') && (length == 14)){
			success = 1;
			*sample_period = (unsigned int) packet[16]; // 1-byte sample period
		}
	}

	return success;
}

/* Parse (Sensing) stop signal from base station
 * - Does not authenticate stop signal yet
 */
int parse_stop(char *packet, unsigned int length, char *origin){
	int success = 0;
	int i;

	// Check frame type
	if (packet[3]==0x90){

		// Check data
		if ((packet[15] == 'X') && (length == 13)){
			success = 1;
		}
	}

	// Extract origin (sender)
	if (success){
		for (i=0; i<8; i++){
			origin[i] = packet[i+4];
		}
	}else{
		for (i=0; i<8; i++){
			origin[i] = 0x00;
		}
	}

	return success;
}

#ifdef MODE_DEBUG
/* Parse Debug mode broadcast signal
 * - Return range: 0 - 15
 */
int parse_debugpacket(char *packet, unsigned int length, unsigned int *num){
	int success = 0;

	// Check frame type
	if (packet[3]==0x90){

		// Check data
		// Debug mode commands
		if ((packet[15] == 'D') && (length == 15)){
			// Check if Broadcast or Unicast
			if (packet[16] == 'B')
				success = 1; // Broadcast
			else if (packet[16] == 'U')
				success = 2; // Unicast
			else if (packet[16] == 'T')
				success = 3; // Change sampling period
			else if (packet[16] == 'P')
				success = 4; // Change power level
			else if (packet[16] == 'C')
				success = 5; // Change channel
			// Extract additional argument
			*num = (unsigned int)packet[17];
		}
		// Change Unicast address
		else if ((packet[15] == 'D') && (length == 22)){
			if (packet[16] == 'A'){
				success = 7;
			}
		}
		// Start normal operation
		else if (packet[15] == 'S') {
			success = parse_start(packet,length,num);
			if (success)
				success = 6; // Start normal sensing
			else
				success = 0;
		}
		// Query parameters
		else if ((packet[15] == 'Q') && (length == 14)){
			// Power level
			if (packet[16] == 'P')
				success = 8;
			// Unicast address
			else if (packet[16] == 'A')
			    success = 9;
			// Sampling period
			else if (packet[16] == 'T')
			    success = 10;
			// Transmit Statistics
			else if (packet[16] == 'S')
			    success = 15;

		}
		// Commit radio settings to NVM
		else if ((packet[15] == 'D') && (length == 14)){
		    if (packet[16] == 'W')
		        success = 11;
		}

		// Change node ID or node loc
		else if (packet[15] == 'D'){
		    if (packet[16] == 'I'){ // Change node ID
		        success = 12;
		    }
		    else if (packet[16] == 'L'){ // Change node loc
		        success = 13;
		    }
		}

		// Enter sleep mode (timed)
		else if ((packet[15] == 'Z') && (length == 15)){
		    // Extract sleep period
		    *num = (unsigned int)(packet[16] << 8) + (unsigned int)packet[17];
		    success = 14;
		}
	}

	return success;

}

/* Change 64-bit address string
 */
void parse_setaddr(char *packet, char *address){
	int i;

	for (i=0; i<8; i++){
		address[i] = packet[i+17];
	}
}

/* Get 64-bit source address field
 */
void parse_srcaddr(char *packet, char *address){
	int i;

	for (i=0; i<8; i++){
		address[i] = packet[i+4];
	}
}

/* Get new node ID from packet and set
 */
unsigned int parse_setnodeid(char *packet, char *node_id){
    unsigned int i, length;

    /* first byte: length */
    length = (unsigned int) packet[17];

    /* succeeding bytes: parameter value */
    for (i=0; i<length; i++){
        node_id[i] = packet[i+18];
    }

    return length;
}

/* Parse AT Command Query response
 */
int parse_atcom_query(char *packet, unsigned int length, int parameter, char *parsedparam){
    int paramlen = 0;

    // Check frame type
    if ((packet[3] == 0x88) && (length >= 5)){

        // Check AT parameter
        // - packet[8] onwards
        switch(parameter){
        case PARAM_PL:
            if ((packet[5] == 'P') && (packet[6] == 'L')){
                parsedparam[0] = packet[8];
                paramlen = 1;
            }
            break;

        case PARAM_WR:
            if ((packet[5] == 'W') && (packet[6] == 'R')){
                parsedparam[0] = packet[7]; //command status
                paramlen = 1;
            }
            break;

        case PARAM_ID:
            if ((packet[5] == 'I') && (packet[6] == 'D')){
                parsedparam[0] = packet[8];
                parsedparam[1] = packet[9];
                paramlen = 2;
            }
        }
    }

    return paramlen;
}
#endif

/* Parse Transmit Status
 * length - header + payload + checksum (3 + x + 1)
 * returns 1 on success
 */
int parse_txstat(char *packet, unsigned int length, char *delivery_p){

    // Check frame type
    if ((packet[3] == 0x8b) && (length == 7)){

        // Delivery status
        *delivery_p = packet[8];

        return 1;
    }else{
        return 0;
    }
}

