/*
 * defines_id.h
 *
 *  Created on: Oct 25, 2019
 *      Author: Snap
 */

const char *node_id_default = "NoName";
const char *node_loc_default = "Nowhere";
unsigned int node_id_len_default = 6; // length of node_id string
unsigned int node_loc_len_default = 7; // length of node_loc string

/* XBee parameter defaults */
//char default_id[] = "\x7f\xff";
#define XBEE_ID 0x7fff;
#define XBEE_CH 0x14;

/* Firmware version (DO NOT edit) */
const char *fwver = "1.7.1";
#define FWVERLEN 5
