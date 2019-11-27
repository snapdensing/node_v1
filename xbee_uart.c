/*
 * xbee_uart.c
 *
 *  Created on: Nov 15, 2019
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"
//#include "parse_routines.c"

int parse_txstat(char *packet, unsigned int length, char *delivery_p);

/* Receive and parse transmit status
 * Return values:
 *   1 - Successful delivery
 *   2 - Received transmit status but failed delivery
 *   0 - Non-transmit status frame received
 * Resets receive buffer after whole frame is received
 */
int rx_txstat(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p, char *rxbuf, unsigned int *fail_ctr_p, unsigned int *tx_ctr_p){

    int parsed_txstat, success;
    char delivery;

    success = 0;

    if (*rxctr_p >= (*rxpsize_p + 4)){

        P3OUT |= 0x40; // UART Rx disable

        parsed_txstat = parse_txstat(rxbuf, *rxpsize_p, &delivery);

        if (parsed_txstat == 1){
            //*state_p = next_state;
            *tx_ctr_p = *tx_ctr_p + 1;

            /* Failed transmit ctr */
            if (delivery != 0x00){
                *fail_ctr_p = *fail_ctr_p + 1;
                success = 2;
            }else{
                success = 1;
            }
        }

        // Reset buffer
        *rxctr_p = 0;
        *rxheader_flag_p = 0;
        *rxpsize_p = 0;
        P3OUT &= 0xbf; // UART Rx Enable

    }

    return success;
}

/* UART transmit: XBee frame
 * Arguments:
 *   tx_buffer - pointer to char array containing frame payload (Xbee frame sans headers + checksum)
 *   length - length of payload
 */
void uarttx_xbee(char *tx_buffer, unsigned int length){
    int i;
    unsigned int checksum;
    char checksum_c;
    checksum = 0;

    /* Send header */
    UCA0TXBUF = 0x7e;
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = (char)(length >> 8); // upper byte
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = (char)(length & 0x00ff); // lower byte
    while (!(IFG2 & UCA0TXIFG));

    /* Send payload and Compute checksum */
    for (i=0; i<length; i++){
        UCA0TXBUF = tx_buffer[i];
        checksum += (unsigned int)tx_buffer[i];
        while (!(IFG2 & UCA0TXIFG));
    }

    /* Send checksum */
    checksum_c = 0xff - (char)(checksum & 0xff);
    UCA0TXBUF = checksum_c;
    while (!(IFG2 & UCA0TXIFG));
}

/* UART assemble transmit request frame (payload only)
 * Arguments:
 *   dest_addr - destination address
 *   data - data to transmit
 *   data_len - length of data
 *   payload - assembled payload
 * Returns:
 *   length of assembled payload
 */
unsigned int assemble_txreq(char *dest_addr, char *data, int data_len, char *payload){
    int i, length;

    /* Frame type */
    payload[0] = 0x10;

    /* Frame ID */
    payload[1] = 0x01;

    /* 64-bit destination address */
    for (i=0; i<8; i++){
        payload[2+i] = dest_addr[i];
    }

    /* Reserved (2 bytes) */
    payload[10] = 0xff;
    payload[11] = 0xfe;

    /* Broadcast radius */
    payload[12] = 0x00;

    /* Transmit options */
    payload[13] = 0x00;

    /* Data */
    length = 14;
    for (i=0; i<data_len; i++){
        payload[14+i] = data[i];
        length++;
    }

    return length;

}
