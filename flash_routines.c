/*
 * flash_routines.c
 *
 *  Created on: Nov 13, 2019
 *      Author: Snap
 */

#include <msp430.h>
#include "defines.h"

void flash_erase(char *addr){
    while (BUSY & FCTL3); // wait until not busy
    FCTL2 = FWKEY | FSSEL_2 | FN1; // SMCLK / 3 (1MHz / 3)
    FCTL3 = FWKEY; // Clear LOCK
    FCTL1 = FWKEY | ERASE; // Enable segment erase
    *addr = 0; // Dummy write to initiate segment erase
    FCTL3 = FWKEY | LOCK; // Set LOCK
}

/* Flash write to whole segment
 * - base_addr must be:
 *     - 0x1000 (segment D)
 *     - 0x1040 (segment C)
 *     - 0x1080 (segment B)
 * - data must be byte array with 64 elements
 */
void segment_wr(char *base_addr, char *data){
    char *addr;
    unsigned int i;

    while (BUSY & FCTL3); // wait until not busy
    FCTL2 = FWKEY | FSSEL_2 | FN1; // SMCLK / 3 (1MHz / 3)
    FCTL3 = FWKEY; // Clear LOCK
    addr = base_addr;
    for (i=0; i<64; i++){
        FCTL1 = FWKEY | WRT; // Enable write
        *addr = data[i]; // Copy byte data
        while (!(WAIT & FCTL3)); // Wait for WAIT to become set
        FCTL1 = FWKEY; // Clear WRT
        while (BUSY & FCTL3); // wait until not busy
        addr++;
    }
    while (BUSY & FCTL3); // wait until not busy
    FCTL3 = FWKEY | LOCK; // Set LOCK
}

/* Assemble 64-byte segment D data
 * Format:
 *   node_id_len  (1 byte)
 *   node_id      (31 bytes)
 *   node_loc_len (1 byte)
 *   node_loc     (31 bytes)
 */
void flash_assemble_segD(char *data, char *node_id, unsigned int node_id_len, char *node_loc, unsigned int node_loc_len){

    unsigned int i;

    /* Initialize data */
    for (i=1; i<64; i++){
        data[i] = 0xff;
    }

    data[0] = (char)(node_id_len & 0x00ff);
    for (i=0; i<node_id_len; i++){
        data[i+1] = node_id[i];
    }

    data[32] = (char)(node_loc_len & 0x00ff);
    for (i=0; i<node_loc_len; i++){
        data[i+33] = node_loc[i];
    }
}

/* Read segment D data
 * Copy flash contents to:
 * - node_id,
 * - node_id_len,
 * - node_loc,
 * - node_loc_len
 */
void read_segD(char *node_id, unsigned int *node_id_lenp, char *node_loc, unsigned int *node_loc_lenp){
    unsigned int i, node_id_len, node_loc_len;

    char *data = (char *)SEG_D;

    node_id_len = (unsigned int)data[0];
    *node_id_lenp = node_id_len;
    for (i=0; i<node_id_len; i++){
        node_id[i] = data[i+1];
    }

    node_loc_len = (unsigned int)data[32];
    *node_loc_lenp = node_loc_len;
    for (i=0; i<node_loc_len; i++){
        node_loc[i] = data[i+33];
    }
}

/* Assemble 64-byte segment C data
 * Format:
 *   valid        (1 byte)
 *   channel      (1 byte)
 *   ID (XBee)    (2 byte)
 *   Aggregator   (8 bytes)
 *   Sampling     (2 bytes)
 */
void flash_assemble_segC(char *data, char *channel, char *panid, char *aggre, unsigned int sampling){

    unsigned int i;

    /* Initialize data */
    for (i=1; i<64; i++){
        data[i] = 0xff;
    }

    /* Valid byte (valid if 0)
     * bit 7 - channel
     * bit 6 - pan ID
     * bit 5 - aggregator
     * bit 4 - sampling
     */
    data[0] = 0x0f;

    /* Channel */
    data[1] = *channel;

    /* Pan ID */
    data[2] = panid[0];
    data[3] = panid[1];

    /* Aggregator */
    for (i=0; i<8; i++){
        data[4+i] = aggre[i];
    }

    /* Sampling period */
    data[12] = (char)(sampling >> 8);
    data[13] = (char)(sampling & 0x00ff);

}

/* Read segment C data
 * Copy flash contents to (if valid):
 * - panid
 * - channel
 * - aggre
 * - sampling
 */
void read_segC(char *validp, char *panid, char *channel, char *aggre, unsigned int *samplingp){
    unsigned int i, sampling;
    char valid;

    char *data = (char *)SEG_C;

    valid = data[0];
    *validp = valid;

    /* Channel */
    if (!(valid & 0x80)){
       *channel = data[1];
    }

    /* Pan ID */
    if (!(valid & 0x40)){
        panid[0] = data[2];
        panid[1] = data[3];
    }

    /* Aggregator */
    if (!(valid & 0x20)){
        for (i=0; i<8; i++){
            aggre[i] = data[4+i];
        }
    }

    /* Sampling period */
    if (!(valid & 0x10)){
        sampling = (unsigned int)data[12];
        sampling = (sampling << 8) + (unsigned int)data[13];
        *samplingp = sampling;
    }

}
