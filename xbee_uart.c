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
