/*
 * transmit_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>

/* AT Command: SH/SL
 * -> 1: SH
 * -> 2: SL
 */
void atcom_shsl(int sel){
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
}

// Transmit Request
void transmitreq(char *tx_data_loc, int tx_data_len_loc, char *tx_dest_loc){
	int packet_len;
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
	/*UCA0TXBUF = 0x01;
	while (!(IFG2&UCA0TXIFG));
	checksum += 0x01;*/
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
	while (!(IFG2&UCA0TXIFG));
}

/* Enable UART RTS control */
void atcom_enrts(void){
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

}

/* Set AT Command PL
 * - make sure val is within 1 byte representation (no error correction yet)
 */
void atcom_pl_set(int val){
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

}
