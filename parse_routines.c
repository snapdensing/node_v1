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
//int parse_atres(char com0, char com1, char *returndata, char *packet, unsigned int length){
int parse_atres(char com0, char com1, char *returndata, char *rxbuf){
	int j;
	if (rxbuf[3]==0x88){

		// Match received with parameter (ignore otherwise)
		if ((rxbuf[5] == com0) && (rxbuf[6] == com1)){

			// Parameter SH
			if ((com0 == 'S') && (com1 == 'H')){
				// Extract upper byte of address
				for (j=0; j<4; j++){
					returndata[j] = rxbuf[j+8];
				}
				return 1;
			}

			// Parameter SL
			else if ((com0 == 'S') && (com1 == 'L')){
				// Extract lower byte of address
				for (j=0; j<4; j++){
					returndata[j+4] = rxbuf[j+8];
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
				returndata[0] = rxbuf[7];
				return 1;
			}

			// Parameter CH
			else if ((com0 == 'C') && (com1 == 'H')){
				// Extract CH parameter value
				returndata[0] = rxbuf[7];
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
/*int parse_ack(char *packet, unsigned int length, char *base_addr){
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
}*/

// Parse (Sensing) start signal from base station
/*int parse_start(char *packet, unsigned int length, unsigned int *sample_period){
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
}*/

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

/* Parse Debug mode broadcast signal
 * - Return range: 0 - 15
 */
int parse_debugpacket(char *packet, unsigned int length, unsigned int *num){
        int success = 0;

        switch(packet[15]){
        case 'D': 
		switch(packet[16]){

		case 'A':
			if (length == 22)
				success = CHGSINK;
			break;

		case 'B':
			if (length == 15){
				success = DBRD;
				*num = (unsigned int) packet[17];
			}
			break;

		case 'C':
			if (length == 15){
				success = CHGCH;
				*num = (unsigned int) packet[17];
			}
			break;

		case 'P':
			if (length == 15){
				success = CHGPL;
				*num = (unsigned int) packet[17];
			}
			break;

		case 'T':
			if (length == 15){
				success = CHGPER;
				*num = (unsigned int) packet[17];
			}
			break;
			
		case 'U':
			if (length == 15){
				success = DUNI;
				*num = (unsigned int) packet[17];
			}
			break;

		case 'I':
			success = CHGID;
			break;

		case 'L':
			success = CHGLOC;
			break;

		case 'W':
			if (length == 14)
				success = COMMIT;
			break;

		case 'F':
		    success = CHGFLAG;
		    *num = (unsigned int) packet[17];
		    break;
		}

		break;

	case 'Q':

		switch(packet[16]){

		case 'A':
			if (length == 14)
				success = QUESINK;
			break;

		case 'P':
			if (length == 14)
				success = QUEPL;
			break;
			
		case 'S':
			if (length == 14)
				success = QUESTAT;
			break;

		case 'T':
			if (length == 14)
				success = QUEPER;
			break;

		case 'F':
		    success = QUEFLAG;
		    break;

		case 'V':
		    success = QUEVER;
		    break;
		}

		break;

	case 'S':

	    switch (length){

	    case 13: // Retain sampling period
	        success = START;
	        break;

	    case 14: // 1-byte sampling period
	        success = START;
	        *num = (unsigned int)packet[16];
	        break;

	    case 15: // 2-byte sampling period
	        success = START;
	        *num = (unsigned int)(packet[16] << 8) + (unsigned int)(packet[17] & 0x00ff);
	        break;

	    }
		break;

	case 'Z':

		if (length == 15){
			success = SLPTIMED;
			*num = (unsigned int)(packet[16] << 8) + (unsigned int)packet[17];
		}
		break;
	}

	/* Check frame type */
	if (packet[3] == 0x90)
		return success;
	else
		return 0;
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

