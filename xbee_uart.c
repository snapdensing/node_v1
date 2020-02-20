/*
 * xbee_uart.c
 *
 *  Created on: Nov 15, 2019
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"

int parse_txstat(char *packet, unsigned int length, char *delivery_p);
int parse_atres(char com0, char com1, char *returndata, char *rxbuf);

/* Reset receive buffer
 * - Includes UART Rx enable
 */
void rst_rxbuf(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p){
    *rxctr_p = 0;
    *rxheader_flag_p = 0;
    *rxpsize_p = 0;
    P3OUT &= 0xbf;
}

/* Receive and parse AT command response
 * Return values:
 *   1 - success
 *   0 - Non-AT command response received
 */
int rx_atres(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p, char *rxbuf, char com0, char com1, char *returndata){

    int success = 0;

    if (*rxctr_p >= (*rxpsize_p + 4)){
        P3OUT |= 0x40; // UART Rx disable
        success = parse_atres(com0, com1, returndata, rxbuf);

        // Reset buffer
        rst_rxbuf(rxheader_flag_p, rxctr_p, rxpsize_p);
    }

    return success;
}

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
        rst_rxbuf(rxheader_flag_p, rxctr_p, rxpsize_p);
        /**rxctr_p = 0;
        *rxheader_flag_p = 0;
        *rxpsize_p = 0;
        P3OUT &= 0xbf;*/ // UART Rx Enable

    }

    return success;
}

/* UART transmit: XBee frame
 * Arguments:
 *   txbuf - pointer to char array containing frame payload (Xbee frame sans headers + checksum)
 *   length - length of payload (does not include headers and checksum)
 */
void uarttx_xbee(char *txbuf, unsigned int length){
    int i;
    char checksum = 0x00;
    char checksum_c;

    /* Send header */
    UCA0TXBUF = 0x7e;
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = (char)(length >> 8); // upper byte
    while (!(IFG2 & UCA0TXIFG));
    UCA0TXBUF = (char)(length & 0x00ff); // lower byte
    while (!(IFG2 & UCA0TXIFG));

    /* Send payload and Compute checksum */
    for (i=0; i<length; i++){
        UCA0TXBUF = txbuf[i];
        //checksum += (unsigned int)txbuf[i];
        checksum += txbuf[i];
        while (!(IFG2 & UCA0TXIFG));
    }

    /* Send checksum */
    //checksum_c = 0xff - (char)(checksum & 0xff);
    checksum_c = 0xff - checksum;
    UCA0TXBUF = checksum_c;
    while (!(IFG2 & UCA0TXIFG));
}

/* UART assemble transmit request frame (payload only)
 * Arguments:
 *   dest_addr - destination address
 *   data - data to transmit
 *   data_len - length of data
 *   txbuf - tx data buffer
 * Returns:
 *   length of assembled payload
 */
void assemble_txreq(char *dest_addr, char *txbuf){
//unsigned int assemble_txreq(char *dest_addr, char *data, int data_len, char *txbuf){
    //unsigned int i, length;
    unsigned int i;

    /* Frame type */
    txbuf[0] = 0x10;

    /* Frame ID */
    txbuf[1] = 0x01;

    /* 64-bit destination address */
    for (i=0; i<8; i++){
        txbuf[2+i] = dest_addr[i];
    }

    /* Reserved (2 bytes) */
    txbuf[10] = 0xff;
    txbuf[11] = 0xfe;

    /* Broadcast radius */
    txbuf[12] = 0x00;

    /* Transmit options */
    txbuf[13] = 0x00;

    /* Data */
    /*length = 14; //initial length (no data yet)
    for (i=0; i<data_len; i++){
        txbuf[14+i] = data[i];
        length++;
    }*/

    /* Data - already assembled prior */
    //length = data_len;

    //return length;
}

/* UART assemble AT command frame
 * Arguments:
 *   atcom - 2-byte AT Command
 *   paramvalue - parameter value in string
 *   paramlen - parameter length
 *   txbuf - tx data buffer
 * Returns:
 *   length of assembled payload
 */
unsigned int assemble_atcom(char *atcom, char *paramvalue, int paramlen, char *txbuf){
    unsigned int i, length;

    /* Frame type */
    txbuf[0] = 0x08;

    /* Frame ID */
    txbuf[1] = 0x01;

    /* AT command */
    txbuf[2] = atcom[0];
    txbuf[3] = atcom[1];
    length = 4;

    /* Parameter value */
    for (i=0; i<paramlen; i++){
        txbuf[4+i] = paramvalue[i];
        length++;
    }

    return length;
}

/* Parameter to AT command LUT
 *
 */

/*void param_to_atcom(int param, char *com0, char *com1){

    switch(param){
    case PARAM_PL:
        *com0 = 'P';
        break;

    case PARAM_WR:
        *com0 = 'W';
        break;

    case PARAM_MR:
        *com0 = 'M';
        break;
    }

    switch(param){
    case PARAM_PL:
        *com1 = 'L';
        break;

    case PARAM_WR:
    case PARAM_MR:
        *com1 = 'R';
        break;
    }
}*/
