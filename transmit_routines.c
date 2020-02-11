/*
 * transmit_routines.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"

void uarttx_xbee(char *txbuf, unsigned int length);
//unsigned int assemble_txreq(char *dest_addr, char *data, int data_len, char *txbuf);
void assemble_txreq(char *dest_addr, char *txbuf);
unsigned int assemble_atcom(char *atcom, char *paramvalue, int paramlen, char *txbuf);

void transmitreq(int tx_data_len, char *dest_addr, char *txbuf){
//void transmitreq(char *tx_data, int tx_data_len, char *dest_addr, char *txbuf){

    //unsigned int i;

    //i = assemble_txreq(dest_addr, tx_data, tx_data_len, txbuf);
    assemble_txreq(dest_addr, txbuf);
    uarttx_xbee(txbuf, tx_data_len);
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

/* Append node_loc and node_id to transmit packet
 * Arguments:
 *   txbuf - transmit buffer
 *   txbuf_i - transmit buffer tail pointer
 *   node_id - node ID pointer
 *   node_id_len - length
 *   node_loc - node loc pointer
 *   node_loc_len - length
 */
void append_nodeinfo(char *txbuf, unsigned int *txbuf_i,
        char *node_id, unsigned int node_id_len, char *node_loc, unsigned int node_loc_len){

    unsigned int i;

    txbuf[*txbuf_i] = 0x07;
    (*txbuf_i)++;
    for (i=0; i<node_id_len; i++){
        txbuf[*txbuf_i] = node_id[i];
        (*txbuf_i)++;
    }

    /* Append node_loc */
    //tx_data[j] = ':';
    txbuf[*txbuf_i] = ':';
    (*txbuf_i)++;
    for (i=0; i<node_loc_len; i++){
        txbuf[*txbuf_i] = node_loc[i];
        (*txbuf_i)++;
    }
}
