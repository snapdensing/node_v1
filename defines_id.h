/*
 * defines_id.h
 *
 *  Created on: Oct 25, 2019
 *      Author: Snap
 */

const char *node_id_default = "Lancelot";
const char *node_loc_default = "Camelot";
unsigned int node_id_len_default = 8; // length of node_id string
unsigned int node_loc_len_default = 7; // length of node_loc string

/* XBee parameter defaults */
//char default_id[] = "\x7f\xff";
#define XBEE_ID 0x7fff;
#define XBEE_CH 0x1a;
