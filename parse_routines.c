/*
 * parse_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"

void parameter_to_str(int parameter, char *atcom);

/* Parse AT Command Response
 */
int parse_atres(int parameter, char *returndata, char *rxbuf, unsigned int *parsedparam_len){
	int j, success;
	char atcom[2];

	success = 0;
	parameter_to_str(parameter, atcom);

	if (rxbuf[3]==0x88){

		// Command status OK and Match received with parameter (ignore otherwise)
		if ((rxbuf[7] == 0x00) && (rxbuf[5] == atcom[0]) && (rxbuf[6] == atcom[1])){

		    // Success return
		    switch(parameter){
		    case PARAM_PL:
		    case PARAM_WR:
		    case PARAM_ID:
		    case PARAM_CH:
		    case PARAM_MR:
		    case PARAM_D6:
		    case PARAM_SH:
		    case PARAM_SL:
		        success = 1;
		        break;
		    }

		    // Return data for queries (ignored by caller if set)
		    switch(parameter){

		    // 4-byte parameter
		    case PARAM_SH:
		    case PARAM_SL:
		        for (j=0; j<4; j++){
                    returndata[j] = rxbuf[j+8];
                }
		        *parsedparam_len = 4;
		        break;

		    // 1-byte parameter
		    case PARAM_PL:
		    case PARAM_CH:
		    case PARAM_MR:
		        returndata[0] = rxbuf[8];
		        *parsedparam_len = 1;
		        break;
		    }
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

/* Parse Debug mode broadcast signal
 * - Return range: 0 - 15
 */
unsigned int parse_debugpacket(char *packet, unsigned int length, unsigned int *num, int *parameter){
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
				*parameter = PARAM_CH;
			}
			break;

		case 'P':
			if (length == 15){
				success = CHGPL;
				*num = (unsigned int) packet[17];
				*parameter = PARAM_PL;
			}else if (length == 16){
			    if (packet[17] == 'L'){
			        success = CHGPL;
			        *num = (unsigned int) packet[18];
			        *parameter = PARAM_PL;
			    }
			}
			break;

		case 'T':
			if (length == 15){
				success = CHGPER;
				*num = (unsigned int) packet[17];
			}else if (length == 16){
			    success = CHGPER;
			    *num = (unsigned int)(packet[17] << 8);
			    *num += (unsigned int)packet[18];
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
			    *parameter = PARAM_WR;
			break;

		case 'F':
		    success = CHGFLAG;
		    *num = (unsigned int) packet[17];
		    break;

		case 'M':
		    if (length == 16){
		        if (packet[17] == 'R'){
		            success = CHGMR;
		            *num = (unsigned int) packet[18];
		            *parameter = PARAM_MR;
		        }
		    }
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
			if (length == 14){
				success = QUEPL;
			    *parameter = PARAM_PL;
			}else if ((length == 15) && (packet[17] == 'L')){
			    success = QUEPL;
			    *parameter = PARAM_PL;
			}
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
		    if (length == 14)
		        success = QUEFLAG;
		    break;

		case 'V':
		    if (length == 14)
		        success = QUEVER;
		    break;

		case 'M':
		    if ((length == 15) && (packet[17] == 'R'))
		        success = QUEMR;
		        *parameter = PARAM_MR;
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

/* Converts integer-encoded AT parameter into string
 *
 */
void parameter_to_str(int parameter, char *atcom){
    /* First byte */
    switch(parameter){
    case PARAM_CH:
        atcom[0] = 'C';
        break;

    case PARAM_D6:
        atcom[0] = 'D';
        break;

    case PARAM_ID:
        atcom[0] = 'I';
        break;

    case PARAM_MR:
        atcom[0] = 'M';
        break;

    case PARAM_PL:
        atcom[0] = 'P';
        break;

    case PARAM_SH:
    case PARAM_SL:
        atcom[0] = 'S';
        break;

    case PARAM_WR:
        atcom[0] = 'W';
        break;

    default:
        atcom[0] = 0x00;
        break;
    }

    /* Second byte */
    switch(parameter){

    case PARAM_ID:
        atcom[1] = 'D';
        break;

    case PARAM_CH:
    case PARAM_SH:
        atcom[1] = 'H';
        break;

    case PARAM_PL:
    case PARAM_SL:
        atcom[1] = 'L';
        break;

    case PARAM_WR:
    case PARAM_MR:
        atcom[1] = 'R';
        break;

    case PARAM_D6:
        atcom[1] = '6';
        break;

    default:
        atcom[1] = 0x00;
        break;
    }
}
