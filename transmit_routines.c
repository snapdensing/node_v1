/*
 * transmit_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"

void uarttx_xbee(char *txbuf, unsigned int length);
unsigned int assemble_txreq(char *dest_addr, char *data, int data_len, char *txbuf);
unsigned int assemble_atcom(char *atcom, char *paramvalue, int paramlen, char *txbuf);

/* AT Command: SH/SL
 * -> 1: SH
 * -> 2: SL
 */
/*void atcom_shsl(int sel){
	int checksum;
	char checksum_c;

	if ((sel==1) || (sel==2)){
		// Wait for empty buffer
		while (!(IFG2&UCA0TXIFG));

		// Start Delimiter
		UCA0TXBUF = 0x7e;
		while (!(IFG2&UCA0TXIFG));

		// Length
		UCA0TXBUF = 0x00; // upper byte
		while (!(IFG2&UCA0TXIFG));
		UCA0TXBUF = 0x04; // lower byte
		while (!(IFG2&UCA0TXIFG));

		// Frame Type
		UCA0TXBUF = 0x08;
		checksum = 0x08;
		while (!(IFG2&UCA0TXIFG));

		// Frame ID
		UCA0TXBUF = 0x01;
		checksum += 0x01;
		while (!(IFG2&UCA0TXIFG));

		// AT Command
		UCA0TXBUF = 'S';
		checksum += (int)'S';
		if (sel==1){
			while (!(IFG2&UCA0TXIFG));
			UCA0TXBUF = 'H';
			checksum += (int)'H';
			while (!(IFG2&UCA0TXIFG));
		}else{
			while (!(IFG2&UCA0TXIFG));
			UCA0TXBUF = 'L';
			checksum += (int)'L';
			while (!(IFG2&UCA0TXIFG));
		}

		// Checksum
		checksum_c = (char)checksum;
		checksum_c = 0xff - checksum_c;
		UCA0TXBUF = checksum_c;
		while (!(IFG2&UCA0TXIFG));
	}
}*/

// Transmit Request
//void transmitreq(char *tx_data_loc, int tx_data_len_loc, char *tx_dest_loc){
void transmitreq(char *tx_data, int tx_data_len, char *dest_addr, char *txbuf){

    unsigned int i;

    i = assemble_txreq(dest_addr, tx_data, tx_data_len, txbuf);
    uarttx_xbee(txbuf, i);

	/*int packet_len;
	int j;
	int checksum;
	char checksum_c;

	// Start Delimiter
	UCA0TXBUF = 0x7e;
	while (!(IFG2&UCA0TXIFG));

	// Length
	packet_len = tx_data_len_loc + 14;
	UCA0TXBUF = (char)(packet_len>>8); // upper byte
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = (char)(packet_len - (packet_len>>8)); // lower byte
	while (!(IFG2&UCA0TXIFG));

	// Frame Type
	UCA0TXBUF = 0x10;
	checksum = 0x10;
	while (!(IFG2&UCA0TXIFG));

	// Frame ID
	UCA0TXBUF = 0x01;
	checksum += 0x01;
	while (!(IFG2&UCA0TXIFG));

	// 64-bit Destination Address
	for (j=0; j<8; j++){
		UCA0TXBUF = tx_dest_loc[j];
		while (!(IFG2&UCA0TXIFG));
		checksum += tx_dest_loc[j];
	}

	// Reserved bytes 13-14
	UCA0TXBUF = 0xff;
	while (!(IFG2&UCA0TXIFG));
	checksum += 0xff;
	UCA0TXBUF = 0xfe;
	while (!(IFG2&UCA0TXIFG));
	checksum += 0xfe;

	// Broadcast Radius
	UCA0TXBUF = 0x00;
	while (!(IFG2&UCA0TXIFG));
	checksum += 0x00;

	// Transmit Options
	UCA0TXBUF = 0x00;
	while (!(IFG2&UCA0TXIFG));
	checksum += 0x00;


	// Data
	for (j=0; j<tx_data_len_loc; j++){
		UCA0TXBUF = tx_data_loc[j];
		while (!(IFG2&UCA0TXIFG));
		checksum += tx_data_loc[j];
	}

	// Checksum
	checksum_c = (char)checksum;
	checksum_c = 0xff - checksum_c;
	UCA0TXBUF = checksum_c;
	while (!(IFG2&UCA0TXIFG));*/
}

/* Enable UART RTS control */
/*void atcom_enrts(void){
	int checksum;
	char checksum_c;

	// Wait for empty buffer
	while (!(IFG2&UCA0TXIFG));

	// Start Delimiter
	UCA0TXBUF = 0x7e;
	while (!(IFG2&UCA0TXIFG));

	// Length
	UCA0TXBUF = 0x00; // upper byte
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = 0x05; // lower byte
	while (!(IFG2&UCA0TXIFG));

	// Frame Type
	UCA0TXBUF = 0x08;
	checksum = 0x08;
	while (!(IFG2&UCA0TXIFG));

	// Frame ID
	UCA0TXBUF = 0x01;
	checksum += 0x01;
	while (!(IFG2&UCA0TXIFG));

	// AT Command
	UCA0TXBUF = 'D';
	checksum += (int)'D';
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = '6';
	checksum += (int)'6';
	while (!(IFG2&UCA0TXIFG));

	// Parameter Value: Enable RTS control
	UCA0TXBUF = 0x01;
	checksum += 0x01;
	while (!(IFG2&UCA0TXIFG));

	// Checksum
	checksum_c = (char)checksum;
	checksum_c = 0xff - checksum_c;
	UCA0TXBUF = checksum_c;
	while (!(IFG2&UCA0TXIFG));

}*/

/* Set AT Command PL
 * - make sure val is within 1 byte representation (no error correction yet)
 */
/*void atcom_pl_set(unsigned int val){
	int checksum;
	char checksum_c;

	// Wait for empty buffer
	while (!(IFG2&UCA0TXIFG));

	// Start Delimiter
	UCA0TXBUF = 0x7e;
	while (!(IFG2&UCA0TXIFG));

	// Length
	UCA0TXBUF = 0x00; // upper byte
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = 0x05; // lower byte
	while (!(IFG2&UCA0TXIFG));

	// Frame Type
	UCA0TXBUF = 0x08;
	checksum = 0x08;
	while (!(IFG2&UCA0TXIFG));

	// Frame ID
	UCA0TXBUF = 0x01;
	checksum += 0x01;
	while (!(IFG2&UCA0TXIFG));

	// AT Command: PL
	UCA0TXBUF = 'P';
	checksum += (int)'P';
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = 'L';
	checksum += (int)'L';
	while (!(IFG2&UCA0TXIFG));

	// Parameter value
	UCA0TXBUF = (char)val;
	checksum += val;
	while (!(IFG2&UCA0TXIFG));

	// Checksum
	checksum_c = (char)checksum;
	checksum_c = 0xff - checksum_c;
	UCA0TXBUF = checksum_c;
	while (!(IFG2&UCA0TXIFG));

}*/

/* Set AT Command CH
 */
/*void atcom_ch_set(unsigned int val){
	int checksum;
	char checksum_c;

	// Wait for empty buffer
	while (!(IFG2&UCA0TXIFG));

	// Start Delimiter
	UCA0TXBUF = 0x7e;
	while (!(IFG2&UCA0TXIFG));

	// Length
	UCA0TXBUF = 0x00; // upper byte
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = 0x05; // lower byte
	while (!(IFG2&UCA0TXIFG));

	// Frame Type
	UCA0TXBUF = 0x08;
	checksum = 0x08;
	while (!(IFG2&UCA0TXIFG));

	// Frame ID
	UCA0TXBUF = 0x01;
	checksum += 0x01;
	while (!(IFG2&UCA0TXIFG));

	// AT Command: CH
	UCA0TXBUF = 'C';
	checksum += (int)'C';
	while (!(IFG2&UCA0TXIFG));
	UCA0TXBUF = 'H';
	checksum += (int)'H';
	while (!(IFG2&UCA0TXIFG));

	// Parameter value
	UCA0TXBUF = (char)val;
	checksum += val;
	while (!(IFG2&UCA0TXIFG));

	// Checksum
	checksum_c = (char)checksum;
	checksum_c = 0xff - checksum_c;
	UCA0TXBUF = checksum_c;
	while (!(IFG2&UCA0TXIFG));

}*/

/* AT command query
 * - Query local AT command parameter value
 */
void atcom_query(int param){
    int checksum;
    char checksum_c;
    char atparam[2];

    // Wait for empty buffer
    while (!(IFG2&UCA0TXIFG));

    // Start Delimiter
    UCA0TXBUF = 0x7e;
    while (!(IFG2&UCA0TXIFG));

    // Length
    UCA0TXBUF = 0x00; //upper
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = 0x04; //lower
    while (!(IFG2&UCA0TXIFG));

    // Frame type
    UCA0TXBUF = 0x08;
    checksum = 0x08;
    while (!(IFG2&UCA0TXIFG));

    // Frame ID
    UCA0TXBUF = 0x01;
    checksum += 0x01;
    while (!(IFG2&UCA0TXIFG));

    // AT Command
    switch (param){
    case PARAM_PL:
        atparam[0] = 'P';
        atparam[1] = 'L';
        break;

    case PARAM_WR:
        atparam[0] = 'W';
        atparam[1] = 'R';
        break;

    default:
        atparam[0] = 0x00;
        atparam[1] = 0x00;
        break;
    }

    UCA0TXBUF = atparam[0];
    checksum += (int)atparam[0];
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = atparam[1];
    checksum += (int)atparam[1];
    while (!(IFG2&UCA0TXIFG));

    // Checksum
    checksum_c = (char)checksum;
    checksum_c = 0xff - checksum_c;
    UCA0TXBUF = checksum_c;
    while (!(IFG2&UCA0TXIFG));

}

/* Debug Query parameter build response
 */
void buildQueryResponse(int param, char *txdata, char *value){
    txdata[0] = 'Q';
    switch(param){
    case PARAM_PL:
        txdata[1] = 'P';
        txdata[2] = 'L';
        txdata[3] = value[0];
        break;
    }
}

/* AT command set
 * - Query local AT command parameter value
 */
/*void atcom_set(int param, char *value){
    int checksum;
    char checksum_c;
    char atparam[2];
    char length[2];

    length[0] = 0x00;
    length[1] = 0x00;

    // Wait for empty buffer
    while (!(IFG2&UCA0TXIFG));

    // Start Delimiter
    UCA0TXBUF = 0x7e;
    while (!(IFG2&UCA0TXIFG));

    // Length
    //UCA0TXBUF = 0x00; //upper
    //while (!(IFG2&UCA0TXIFG));
    //UCA0TXBUF = 0x04; //lower
    //while (!(IFG2&UCA0TXIFG));
    switch (param){

    case PARAM_ID:
        length[0] = 0x00;
        length[1] = 0x06;
        break;

    case PARAM_CH:
        length[0] = 0x00;
        length[1] = 0x05;

    default:
        length[0] = 0x00;
        length[1] = 0x04;
        break;
    }
    UCA0TXBUF = length[0];
    while (!(IFG2&UCA0TXIFG))
    UCA0TXBUF = length[1];
    while (!(IFG2&UCA0TXIFG))

    // Frame type (1 byte)
    UCA0TXBUF = 0x08;
    checksum = 0x08;
    while (!(IFG2&UCA0TXIFG));

    // Frame ID (1 byte)
    UCA0TXBUF = 0x01;
    checksum += 0x01;
    while (!(IFG2&UCA0TXIFG));

    // AT Command (2 bytes)
    switch (param){
    case PARAM_ID:
        atparam[0] = 'I';
        atparam[1] = 'D';
        break;

    case PARAM_CH:
        atparam[0] = 'C';
        atparam[1] = 'H';
        break;

    default:
        atparam[0] = 0x00;
        atparam[1] = 0x00;
        break;
    }
    UCA0TXBUF = atparam[0];
    checksum += (int)atparam[0];
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = atparam[1];
    checksum += (int)atparam[1];
    while (!(IFG2&UCA0TXIFG));

    // Parameter value
    switch(param){

    case PARAM_ID: // ID (2 bytes)
        UCA0TXBUF = value[0];
        checksum += (int)value[0];
        while (!(IFG2&UCA0TXIFG));
        UCA0TXBUF = value[1];
        checksum += (int)value[1];
        while (!(IFG2&UCA0TXIFG));
        break;

    case PARAM_CH: // CH (1 byte)
        UCA0TXBUF = value[0];
        checksum += (int)value[0];
        while (!(IFG2&UCA0TXIFG));
        break;

    }

    // Checksum
    checksum_c = (char) checksum;
    checksum_c = 0xff - checksum_c;
    UCA0TXBUF = checksum_c;
    while (!(IFG2&UCA0TXIFG));

}*/

/* Set AT Command ID
 */
/*void atcom_id_set(unsigned int val){
    int checksum;
    char checksum_c;

    // Wait for empty buffer
    while (!(IFG2&UCA0TXIFG));

    // Start Delimiter
    UCA0TXBUF = 0x7e;
    while (!(IFG2&UCA0TXIFG));

    // Length
    UCA0TXBUF = 0x00; // upper byte
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = 0x06; // lower byte
    while (!(IFG2&UCA0TXIFG));

    // Frame Type
    UCA0TXBUF = 0x08;
    checksum = 0x08;
    while (!(IFG2&UCA0TXIFG));

    // Frame ID
    UCA0TXBUF = 0x01;
    checksum += 0x01;
    while (!(IFG2&UCA0TXIFG));

    // AT Command: ID
    UCA0TXBUF = 'I';
    checksum += (int)'I';
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = 'D';
    checksum += (int)'D';
    while (!(IFG2&UCA0TXIFG));

    // Parameter value
    UCA0TXBUF = (char)(val >> 8);
    checksum += (char)(val >> 8);
    while (!(IFG2&UCA0TXIFG));
    UCA0TXBUF = (char)(val & 0x00ff);
    checksum += (char)(val & 0x00ff);
    while (!(IFG2&UCA0TXIFG));

    // Checksum
    checksum_c = (char)checksum;
    checksum_c = 0xff - checksum_c;
    UCA0TXBUF = checksum_c;
    while (!(IFG2&UCA0TXIFG));

}*/

/* Transmit AT command
 * Arguments:
 *   com0,com1 - AT parameter
 *   paramvalue - string of parameter value (for set)
 *   paramlen - paramvalue length. Equal to 0 for query
 *   txbuf - transmit buffer
 */
void atcom(char com0, char com1, char *paramvalue, int paramlen, char *txbuf){

    unsigned int i;
    char atcom[2];

    atcom[0] = com0;
    atcom[1] = com1;

    i = assemble_atcom(atcom, paramvalue, paramlen, txbuf);
    uarttx_xbee(txbuf, i);
}
