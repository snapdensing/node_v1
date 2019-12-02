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

void transmitreq(char *tx_data, int tx_data_len, char *dest_addr, char *txbuf){

    unsigned int i;

    i = assemble_txreq(dest_addr, tx_data, tx_data_len, txbuf);
    uarttx_xbee(txbuf, i);
}

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
