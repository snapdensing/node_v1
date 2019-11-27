#include <msp430.h> 
#include "defines.h"
#include "defines_id.h"

/* Function Prototypes */

int parse_header(void);

int parse_atres(char com0, char com1, char *returndata, char *packet, unsigned int length);

int parse_ack(char *packet, unsigned int length, char *base_addr);
int parse_start(char *packet, unsigned int length, unsigned int *sample_period);
int parse_stop(char *packet, unsigned int length, char *origin);

#ifdef MODE_DEBUG
int parse_debugpacket(char *packet, unsigned int length, unsigned int *num);
void parse_setaddr(char *packet, char *address);
void parse_srcaddr(char *packet, char *address);
int parse_atcom_query(char *packet, unsigned int length, int parameter, char *parsedparam);
unsigned int parse_setnodeid(char *packet, char *node_id);
#endif

#ifdef SENSOR_BATT
int buildSense(char *tx_data, unsigned int sensor_flag, unsigned int tx_count, unsigned int *batt_out);
unsigned int Battery_supply_nonreg(void);
#else
int buildSense(char *tx_data, unsigned int sensor_flag, unsigned int tx_count);
#endif

void atcom_shsl(int sel);
void transmitreq(char *tx_data_loc, int tx_data_len_loc, char *tx_dest_loc);
void atcom_enrts(void);
void atcom_pl_set(int val);
void atcom_ch_set(int val);
void atcom_query(int param);
void atcom_set(int param, char *value); //buggy
void atcom_id_set(unsigned int val);

unsigned int assemble_txreq(char *dest_addr, char *data, int data_len, char *payload);
void uarttx_xbee(char *tx_buffer, unsigned int length);

int check_sensor(int sensor_id);
//unsigned int detect_sensor(void);
void detect_sensor(unsigned int *sensor_flagp);

void flash_erase(char *addr);
void segment_wr(char *base_addr, char *data);
void flash_assemble_segD(char *data, char *node_id, unsigned int node_id_len, char *node_loc, unsigned int node_loc_len);
void read_segD(char *node_id, unsigned int *node_id_lenp, char *node_loc, unsigned int *node_loc_lenp);
void flash_assemble_segC(char *data, char *channel, char *panid, char *aggre, unsigned int sampling);
void read_segC(char *validp, char *panid, char *channel, char *aggre, unsigned int *samplingp);

int rx_txstat(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p, char *rxbuf, unsigned int *fail_ctr_p, unsigned int *tx_ctr_p);

/* Global Variables */

/** Receive Buffer **/
char rxbuf[RXBUF_MAX];	//receive shift register
unsigned int rxctr;		//received bytes counter
unsigned int rxpsize;	//received packet size
int rxheader_flag;		//receiving frame headers flag (1 when done)
int rxbuf_overflow;

/** Timer **/
unsigned int timer_flag;

/** State Encoding **/
int state;

/** Constant Addresses **/
//static char broadcast_addr[] = "\x00\x00\x00\x00\x00\x00\xff\xff"; //broadcast
//static char unicast_addr_default[]   = "\x00\x13\xA2\x00\x40\x9A\x0A\x81"; //unicast address (test)

/** Constant messages **/
//char stopACK[] = "XA"; // Stop command acknowledge
//char startACK[] = "SA"; // Start command acknowledge

/* Main */

int main(void) {

    /** Constant Addresses **/
    static char broadcast_addr[] = "\x00\x00\x00\x00\x00\x00\xff\xff"; //broadcast
    static char unicast_addr_default[]   = "\x00\x13\xA2\x00\x40\x9A\x0A\x81"; //unicast address (test)

    /** Constant messages **/
    static char stopACK[] = "XA"; // Stop command acknowledge
    static char startACK[] = "SA"; // Start command acknowledge

	/* Global Variables */

	/** Local counters **/
	int i,j;

	/** Address Storage **/
	char node_address[8]; // Node XBee address
	char unicast_addr[8]; // Base station address
	char origin_addr[8]; //

	/** Node ID **/
	char node_id[MAXIDLEN];
	char node_loc[MAXLOCLEN];
	unsigned int node_id_len, node_loc_len;


	/** Sleep mode **/
	unsigned int sleep_time;

	/** Transmit buffer **/
	char tx_data[MAXDATA];
	char payload[MAXPAYLOAD];
	unsigned int tx_count;
#ifdef MODE_DEBUG
	unsigned int temp_uint;
	char atres_status;
	int parameter;
	char parsedparam[8];
	char channel; // XBee channel
	char panid[2]; // XBee ID

	unsigned int standby_sample = STANDBY_SAMPLEBATT; // Standby (Debug state) battery sample period

	/** Flash **/
	char flash_data[64];
	char *flash_addr;
	char valid_segC;
    unsigned int test_id_len;
    unsigned int test_loc_len;
    char test_id[31], test_loc[31];
#endif


	/** Configuration and flags **/
	unsigned int sample_period;
	int stop_flag;
	unsigned int sensor_flag;

	/* Peripheral Setup */

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    if (CALBC1_1MHZ==0xFF) {	// If calibration constant erased
          while(1);					// do not load, trap CPU!!
    }

    /** clock setup **/
    DCOCTL = 0;                               // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;                    // Set DCO to 1MHz
    DCOCTL = CALDCO_1MHZ;

    /** uart setup **/
    P3SEL = 0x30;                             // P3.4,5 = USCI_A0 TXD/RXD
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 104;                            // 1MHz 9600
    UCA0BR1 = 0;                              // 1MHz 9600
    UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    IE2 |= UCA0RXIE;                          // Enable USCI_A0 RX interrupt

    /** UART RTS control **/
    P3DIR |= 0x40;							// P3.6 = RTS control output
    //P3OUT &= 0xbf;							// nRTS to 0 (UART Rx enable)
    P3OUT |= 0x40;							// nRTS to 1 (UART Rx disable)

    /** timer setup **/
    TA0CCTL0 = CCIE;                        // TA0CCR0 interrupt enabled
    TA0CCR0 = 62500;						//triggered every 500000 cycles (approx 0.5 seconds @ 1MHz)
    TA0CTL = TASSEL_2 + MC_2 + ID_2;        // SMCLK, contmode, divide clock /8

    /** Sleep-related I/O setup **/
    P1DIR &= 0xfd; // set P1.2 to input (XBEE ON/SLEEP indicator)
    P3DIR |= 0x80; // set P3.7 to output (XBEE SLEEP_RQ)
    P3OUT &= 0x7f; // set P3.7 output to 0 (XBEE awake)

    /** Charger **/
    P1SEL &= 0xdf; // Set P1.5 as GPIO
    P1DIR |= 0x20; // Set P1.5 as output
    //P1OUT |= 0x20; // Set P1.5 to enable charging
    P1OUT &= 0xdf; // Reset P1.5 to disable charging
    int charge_flag = 0; // Set to 1 if node is in charging mode
    unsigned int batt; // Battery voltage
    unsigned int batt_hi = BATT_VTHHI;
    unsigned int batt_lo = BATT_VTHLO;

    /** Debug information **/
    unsigned int sensetx_fail = 0;
    unsigned int sensetx = 0;


    /* Initialization */

    /** Rx Buffer Initialization **/
    for (i=0; i<sizeof(rxbuf); i++){
        rxbuf[i] = 0x00;
    }
    rxctr = 0;
    rxheader_flag = 0;
    rxpsize = 0;
    rxbuf_overflow = 0;

    /** State **/
    state = S_RTS1;

    /** Tx Buffer **/
    tx_count = 0;
#ifdef MODE_DEBUG
    temp_uint = 0;
#endif

    /** Check sensors **/
    //sensor_flag = detect_sensor();
    //detect_sensor(&sensor_flag);

    /* Default sampling period
     * - possible overwrite by flash if value exists
     */
    sample_period = SAMPLE_PERIOD;

    /* Initialize Node ID/Loc */
    read_segD(test_id, &test_id_len, test_loc, &test_loc_len);

    // Node ID
    if (test_id_len < MAXIDLEN){ // Get from flash
        node_id_len = test_id_len;
        for (i=0; i<node_id_len; i++)
            node_id[i] = test_id[i];
    }else{ // Use programming defaults
        node_id_len = node_id_len_default;
        for (i=0; i<node_id_len_default; i++)
            node_id[i] = node_id_default[i];
    }
    // Node Loc
    if (test_loc_len < MAXLOCLEN){ // Get from flash
        node_loc_len = test_loc_len;
        for (i=0; i<node_loc_len; i++)
            node_loc[i] = test_loc[i];
    }else{
        node_loc_len = node_loc_len_default;
        for (i=0; i<node_loc_len_default; i++)
            node_loc[i] = node_loc_default[i];
    }

    /* Initialize Default unicast address
     * - Possible overwrite by flash if value exists
     */
    for (i=0; i<8; i++){
        origin_addr[i] = unicast_addr_default[i];
        unicast_addr[i] = unicast_addr_default[i];
    }

    /* Flash Segment C read for initialization */
    read_segC(&valid_segC, panid, &channel, unicast_addr, &sample_period);

    /** Check sensors **/
    detect_sensor(&sensor_flag);

    /* Start */

    /** Enable Interrupts **/
    __bis_SR_register(GIE);

    while(1){
    	switch(state){

    	/** State: Enable RTS control (AT Command Send) **/
    	case S_RTS1:

    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_RTS2;
    		atcom_enrts();

    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    		break;

    	/** State: Enable RTS control (AT Command Response) **/
    	case S_RTS2:

    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
    			if (rxctr >= (rxpsize + 4)){
    				j = parse_atres('D','6',node_address,rxbuf,rxpsize);
    				//if (j == 1){
    					state = S_ADDR1;
    				//}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;

    				P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    			}
    		}
    		break;

    	/** State: Get Address (AT Command Send Upper) **/
    	case S_ADDR1:

    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_ADDR2;
    		atcom_shsl(1);

    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    		break;

    	/** State: Get Address (AT Command Response Upper) **/
    	case S_ADDR2:

    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
    			if (rxctr >= (rxpsize + 4)){ // Entire packet received

    			    P3OUT |= 0x40; // UART Rx disable

    				j = parse_atres('S','H',node_address,rxbuf,rxpsize);
    				if (j == 1){
    					state = S_ADDR3;
    				}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;
    				P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    			}
    		}
    		break;

    	/** State: Get Address (AT Command Send Lower) **/
    	case S_ADDR3:

    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_ADDR4;
    		atcom_shsl(2);

    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    		break;

    	/** State: Get Address (AT Command Response Lower) **/
    	case S_ADDR4:
    		if (rxheader_flag == 0){
    			parse_header();
    		}
    		else{
    			if (rxctr >= (rxpsize + 4)){ // Entire packet received

    			    P3OUT |= 0x40; // UART Rx disable

    				j = parse_atres('S','L',node_address,rxbuf,rxpsize);
    				if (j == 1){
#ifdef MODE_DEBUG
    					//state = S_DEBUG;
    				    state = S_BOOTUP1;
#else
    					//state = S_INIT;
    					state = S_SENSE;
#endif
    				}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;
    				P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    			}
    		}
    		break;

    	/** State: Bootup - set ID **/
    	case S_BOOTUP1:
    	    P3OUT |= 0x40;  // nRTS to 1 (UART Rx disable)

    	    state = S_BOOTUP2;
    	    //parameter = PARAM_ID;
    	    //atcom_set(parameter,default_id);
    	    /* Get value from flash if it exists */
    	    if (!(valid_segC & 0x40)){
    	        temp_uint = (unsigned int)panid[0];
    	        temp_uint = (temp_uint << 8) + (unsigned int)panid[1];
    	    }else{
    	        temp_uint = XBEE_ID;
    	        panid[0] = (char)(temp_uint >> 8);
    	        panid[1] = (char)(temp_uint & 0x00ff);
    	    }
    	    atcom_id_set(temp_uint);

    	    P3OUT &= 0xbf;  // nRTS to 0 (UART Rx enable)
    	    break;

    	/** State: Bootup - set ID response **/
    	case S_BOOTUP2:
            if (rxheader_flag == 0){
                parse_header();
            }
            else{
                if (rxctr >= (rxpsize + 4)){

                    P3OUT |= 0x40; // UART Rx disable

                    j = parse_atres('I','D',node_address,rxbuf,rxpsize);
                    if (j == 1){
                        state = S_BOOTUP3;
                    }

                    // Reset buffer
                    rxctr = 0;
                    rxheader_flag = 0;
                    rxpsize = 0;
                    P3OUT &= 0xbf; // UART Rx enable
                }
            }
            break;

        /** State: Bootup - set CH **/
        case S_BOOTUP3:
            P3OUT |= 0x40;  // nRTS to 1 (UART Rx disable)

            state = S_BOOTUP4;
            /* Get value from flash if it exists */
            if (!(valid_segC & 0x80)){
                temp_uint = channel;
            }else{
                channel = (char)XBEE_CH;
                temp_uint = XBEE_CH;
            }
            atcom_ch_set(temp_uint);

            P3OUT &= 0xbf;  // nRTS to 0 (UART Rx enable)
            break;

        /** State: Bootup - set CH response **/
        case S_BOOTUP4:
            if (rxheader_flag == 0){
                parse_header();
            }
            else{
                if (rxctr >= (rxpsize + 4)){

                    P3OUT |= 0x40; // UART Rx disable

                    j = parse_atres('C','H',node_address,rxbuf,rxpsize);
                    if (j == 1){
                        state = S_DEBUG;
                    }

                    // Reset buffer
                    rxctr = 0;
                    rxheader_flag = 0;
                    rxpsize = 0;
                    P3OUT &= 0xbf; // UART Rx enable
                }
            }
            break;

    	/** State: Perform sensing and transmit **/
    	case S_SENSE:
    		/* Sense and transmit
    		 * - use constant SAMPLE_PERIOD if hardcoded period in defines.h
    		 * - use variable sample_period if base station configured period
    		 */

   			P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

   			//state = NS_SENSE;
   			state = S_SENSERES;

   			/* Assemble Packet Data */
#ifdef SENSOR_BATT
   			j = buildSense(tx_data,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}

   			tx_data[j] = 0x07;
   			j++;
   			/* Append node_id */
   			for (i=0; i<node_id_len; i++){
   			    tx_data[j] = node_id[i];
   			    j++;
   			}
   			tx_data[j] = ':';
   			j++;

   			/* Append node_loc */
   			for (i=0; i<node_loc_len; i++){
   			    tx_data[j] = node_loc[i];
   			    j++;
   			}

#ifdef DEBUG_CHARGING
   			/* Append charge_flag to packet */
   			tx_data[j] = (char)(charge_flag & 0x00ff);
   			j++;
#endif

#else
   			j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
#endif

#ifdef SENSOR_BATT
            /* Charging state */
            if (charge_flag == 0){ // Discharging
                //if (batt < BATT_VTHLO){
                if (batt < batt_lo){
                    charge_flag = 1;
                    P1OUT |= 0x20; // Set P1.5 to enable charging
                }
            }
            else{ // Charging
                //if (batt > BATT_VTHHI){
                if (batt > batt_hi){
                    charge_flag = 0;
                    P1OUT &= 0xdf; // Reset P1.5 to disable charging
                }
            }
#endif

   			/* Transmit */
#ifdef BROADCAST
   			transmitreq(tx_data, j, broadcast_addr);
#else
   			transmitreq(tx_data, j, unicast_addr);
#endif

   			/* Update Transmit Counter */
    		tx_count++;

   			/* Reset Timer flag */
    		//timer_flag = 0;

   			P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

   			/* Reset Stop flag for Stop Window state */
			stop_flag = 0;

    		break;

    	/** State: Sense response (catch transmit status) **/
    	case S_SENSERES:

    	    if (rxheader_flag == 0){
    	        parse_header();
    	    }else{
    	        j = rx_txstat(&rxheader_flag, &rxctr, &rxpsize, rxbuf, &sensetx_fail, &sensetx);
    	        if (j != 0){
    	            state = S_WINDOW;
    	        }
    	    }

    	    break;

    	/** State: Window for stopping node sensing **/
    	case S_WINDOW:

    		// Receive and process XBee frame
    		if (rxheader_flag == 0){
    		    parse_header();
    		}else{
    		    if (rxctr >= (rxpsize + 4)){

    		        P3OUT |= 0x40; // (UART Rx disable)

    		        stop_flag = parse_stop(rxbuf, rxpsize, origin_addr);

    		        // Reset buffer
    		        rxctr = 0;
    		        rxheader_flag = 0;
    		        rxpsize = 0;
    		        P3OUT &= 0xbf; // (UART Rx enable)
    		    }
    		}

    		// Determine next state
    		if (stop_flag == 1){
    		    state = S_STOP;
    		}else{
    		    if (timer_flag < sample_period){
    		        state = S_WINDOW;
    		    }else{
    		        state = S_SENSE;
    		        timer_flag = 0; // Reset timer flag
    		    }
    		}

    		break;

    	/** State: Stop signal acknowledge **/
    	case S_STOP:
    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_STOPRES;

    		/* Transmit */
   			transmitreq(stopACK, 2, origin_addr);

    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    		break;

    	/** State: Stop signal acknowledge response **/
    	case S_STOPRES:

    	    if (rxheader_flag == 0){
    	        parse_header();
    	    }else{
    	        j = rx_txstat(&rxheader_flag, &rxctr, &rxpsize, rxbuf, &sensetx_fail, &sensetx);
    	        if (j == 1){
    	            state = S_DEBUG;
    	        }else if (j != 0){
    	            state = S_STOP; // Re-transmit stopACK
    	        }
    	    }

    	    break;

    	/** State: Start signal acknowledge **/
    	case S_START:
    	    P3OUT |= 0x40;  // nRTS to 1 (UART Rx disable)

    	    //state = S_SENSE;
    	    state = S_STARTRES;

    	    /* Transmit */
    	    transmitreq(startACK, 2, origin_addr);

    	    P3OUT &= 0xbf;  // nRTS to 0 (UART Rx enable)
    	    break;

    	/** State: Start signal acknowledge response **/
    	case S_STARTRES:

    	    if (rxheader_flag == 0){
    	        parse_header();
    	    }else{
    	        j = rx_txstat(&rxheader_flag, &rxctr, &rxpsize, rxbuf, &sensetx_fail, &sensetx);
    	        if (j == 1){
    	            state = S_SENSE; // Loop until successful ack transmit

    	            /* Reset statistics */
    	            sensetx = 0;
    	            sensetx_fail = 0;
    	        }else if (j != 0){
    	            state = S_START; // re-transmit  startACK
    	        }
    	    }

    	    break;

#ifdef MODE_DEBUG
    	/** State: Debug mode entrypoint **/
    	case S_DEBUG:

#ifdef SENSOR_BATT
    		/* Standby battery monitoring */
    		if (timer_flag > standby_sample){
    			batt = Battery_supply_nonreg();
    			timer_flag = 0; // Reset timer flag
    		}
    		if (charge_flag == 0){ // Discharging
    		    if (batt < batt_lo){
    		        charge_flag = 1;
    		        P1OUT |= 0x20; // Set P1.5 to enable charging
    		    }
    		}
    		else{ // Charging
    		    if (batt > batt_hi){
    		        charge_flag = 0;
    		        P1OUT &= 0xdf; // Reset P1.5 to disable charging
    		    }
    		}
#endif

    		/* Process packet received */
    		if (rxheader_flag == 0){
    			parse_header();

    		}else{

    			if (rxctr >= (rxpsize + 4)){ // Entire packet received

    				j = parse_debugpacket(rxbuf, rxpsize, &temp_uint);

    				tx_count = 0;

    				switch(j){
    				//if (j != 0){
    					//timer_flag = 0; // Reset timer flag
    					//tx_count = 0;

    					// Debug Broadcast
    					//if (j == DBRD){
    					case DBRD:
    						timer_flag = 0; // Reset timer flag
    						state = NS_DEBUG1;
    						break;
    					//}

    					// Debug Unicast
    					//else if (j == DUNI){
    					case DUNI:
    						timer_flag = 0; // Reset timer flag
    						state = NS_DEBUG2;
    						parse_srcaddr(rxbuf,origin_addr);
    						break;
    					//}

    					// Change Period
    					//else if (j == CHGPER){
    					case CHGPER:
    						//state = NS_DEBUG3;
    					    state = S_DEBUG;
    						sample_period = temp_uint;
    						break;
    					//}

    					// Change PL
    					//else if (j == CHGPL){
    					case CHGPL:
    						//state = NS_DEBUG4;
    					    state = S_DPLRES;
    			    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)
    			    		atcom_pl_set(temp_uint);
    			    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    			    		break;
    					//}

    					// Change CH
    					//else if (j == CHGCH){
    					case CHGCH:
    						//state = NS_DEBUG5;
    					    state = S_DCHRES;
    					    channel = (char)(temp_uint & 0x00ff);
    						P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)
							atcom_ch_set(temp_uint);
							P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
							break;
    					//}

    					// Start sensing
    					//else if (j == START){
    					case START:
    						//state = S_SENSE;
    					    state = S_START;
    						sample_period = temp_uint;
    						parse_srcaddr(rxbuf,origin_addr);
    						break;
    					//}

    					// Change unicast address
    					case CHGSINK:
    					//else if (j == CHGSINK){
    						state = S_DEBUG;
    						parse_setaddr(rxbuf,unicast_addr);
    						break;
    					//}

    					// Query power level
    					case QUEPL:
    					//else if (j == QUEPL){
    						state = S_DQRES1;
    						parameter = PARAM_PL;
    						parse_srcaddr(rxbuf,origin_addr);
    						break;
    					//}

    					// Query unicast address
    					case QUESINK:
    					//else if (j == QUESINK){
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    tx_data[0] = 'Q';
    					    tx_data[1] = 'A';
    					    for (i=0; i<8; i++)
    					        tx_data[i+2] = unicast_addr[i];
    					    j = 10; // tx_data length
    					    break;
    					//}

    					// Query sampling period
    					case QUEPER:
    					//else if (j == QUEPER){
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    tx_data[0] = 'Q';
    					    tx_data[1] = 'T';
    					    tx_data[2] = (char)sample_period; //assumes period is only 8-bits
    					    j = 3; // tx_data length
    					    break;
    					//}

    					// Commit radio settings to NVM
    					case COMMIT:
    					//else if (j == COMMIT){
    					    state = S_DQRES1;
    					    parameter = PARAM_WR;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    break;
    					//}

    					// Change node ID
    					case CHGID:
    					//else if (j == CHGID){
    					    state = S_DEBUG;
    					    node_id_len = parse_setnodeid(rxbuf,node_id);
    					    break;
    					//}

    					// Change node loc
    					case CHGLOC:
    					//else if (j == CHGLOC){
    					    state = S_DEBUG;
    					    node_loc_len = parse_setnodeid(rxbuf,node_loc);
    					    break;
    					//}

    					// Enter timed sleep
    					case SLPTIMED:
    					//else if (j == SLPTIMED){
    					    state = S_SLEEP1;
    					    sleep_time = temp_uint;
    					    timer_flag = 0;
    					    P3OUT |= 0x80; // high to sleep XBee
    					    break;
    					//}

    					// Query statistics
    					case QUESTAT:
    					//else if (j == QUESTAT){
    					    state = S_DQRES3;
                            parse_srcaddr(rxbuf,origin_addr);
                            tx_data[0] = 'Q';
                            tx_data[1] = 'S';
                            // 1st parameter: sensetx
                            tx_data[2] = (char)(sensetx >> 8);
                            tx_data[3] = (char)(sensetx & 0x00ff);
                            // 2nd parameter: sensetx_fail
                            tx_data[4] = (char)(sensetx_fail >> 8);
                            tx_data[5] = (char)(sensetx_fail & 0x00ff);
                            j = 6; // tx_data length
                            break;
    					//}
    				}
    				//}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;
    				P3OUT &= 0xbf; // nRTS to 0 (UART enable)
    			}

    		}
    		break;

    	/** State: Debug mode Broadcast **/
    	case S_DBRD:
    		if (timer_flag > sample_period){

    			P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    			/* Update Transmit Counter */
    			tx_count++;
    			if (tx_count < temp_uint){
    				state = NS_DBRDLOOP;
    			}else{
    				state = NS_DBRDBRK;
    			}

    			/* Assemble Packet Data */
#ifdef SENSOR_BATT
    			j = buildSense(tx_data,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
#else
                j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
#endif

                /* Append node_id */
                tx_data[j] = 0x07;
                j++;
                for (i=0; i<node_id_len; i++){
                    tx_data[j] = node_id[i];
                    j++;
                }
                tx_data[j] = ':';
                j++;

                /* Append node_loc */
                for (i=0; i<node_loc_len; i++){
                    tx_data[j] = node_loc[i];
                    j++;
                }

    			/* Transmit */
    			transmitreq(tx_data, j, broadcast_addr);

    			/* Reset Timer flag */
    			timer_flag = 0;

    			P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    		}
    		break;

    	/** State: Debug mode Unicast **/
    	case S_DUNI:
    		if (timer_flag > sample_period){

    		    P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		    /* Update Transmit Counter */
    		    tx_count++;
    		    if (tx_count < temp_uint){
    		    	state = NS_DUNILOOP;
    		    }else{
    		    	state = NS_DUNIBRK;
    		    }

    		    /* Assemble Packet Data */
#ifdef SENSOR_BATT
    		    j = buildSense(tx_data,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
#else
    		    j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
#endif

                /* Append node_id */
    		    tx_data[j] = 0x07;
    		    j++;
                for (i=0; i<node_id_len; i++){
                    tx_data[j] = node_id[i];
                    j++;
                }
                tx_data[j] = ':';
                j++;

                /* Append node_loc */
                for (i=0; i<node_loc_len; i++){
                    tx_data[j] = node_loc[i];
                    j++;
                }

    		    /* Transmit */
    		    transmitreq(tx_data, j, origin_addr);

    		    /* Reset Timer flag */
    		    timer_flag = 0;

    		    P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    		}
    		break;

    	/* State: Debug wait for PL AT command response */
    	case S_DPLRES:

    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
    			if (rxctr >= (rxpsize + 4)){

    				j = parse_atres('P','L',&atres_status,rxbuf,rxpsize);
    				if (j == 1){
    					//state = NS_DPLRES;
    				    state = S_DEBUG;
    				}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;
    				P3OUT &= 0xbf; // nRTS to 0 (UART Rx enable)
    			}
    		}
    		break;

		/* State: Debug wait for CH AT command response */
		case S_DCHRES:

			if (rxheader_flag == 0){
				parse_header();
			}else{
				if (rxctr >= (rxpsize + 4)){

					j = parse_atres('C','H',&atres_status,rxbuf,rxpsize);
					if (j == 1){
						state = NS_DCHRES;
					}

					// Reset buffer
					rxctr = 0;
					rxheader_flag = 0;
					rxpsize = 0;
					P3OUT &= 0xbf; // nRTS to 0 (UART Rx enable)
				}
			}
			break;

		/* State: Debug Query parameter */
		case S_DQRES1:
		    P3OUT |= 0x40; // UART Rx disable
		    atcom_query(parameter);
		    P3OUT &= 0xbf; // UART Rx enable
		    state = S_DQRES2;
		    break;

		/* State: Debug Query parameter response */
		case S_DQRES2:
		    if (rxheader_flag == 0){
		        parse_header();
		    }
		    else{
		        if (rxctr >= (rxpsize + 4)){

		            P3OUT |= 0x40; // UART Rx disable

		            j = parse_atcom_query(rxbuf, rxpsize, parameter, parsedparam); // j is parsed parameter length
		            if (j > 0){
		                tx_data[0] = 'Q';
		                switch(parameter){
		                case PARAM_PL:
		                    tx_data[1] = 'P';
		                    tx_data[2] = 'L';
		                    tx_data[3] = parsedparam[0];
		                    j = 4; // length of tx_data
	                        state = S_DQRES3;
		                    break;

		                case PARAM_WR:
		                    tx_data[1] = 'W';
		                    tx_data[2] = 'R';
		                    tx_data[3] = parsedparam[0];
		                    j = 4;
		                    state = S_DQRES3;
		                    break;

		                default:
		                    state = S_DEBUG;
		                    break;
		                }
		            }else{
		                state = S_DEBUG;
		            }

		            // Reset buffer
		            rxctr = 0;
		            rxheader_flag = 0;
		            rxpsize = 0;
		            P3OUT &= 0xbf; // UART Rx enable
		        }
		    }
		    break;

	    /* State: Debug Transmit Query parameter response */
	    case S_DQRES3:

	        // Save MSP Parameters to flash
            if (parameter == PARAM_WR){
                flash_assemble_segD(flash_data, node_id, node_id_len, node_loc, node_loc_len);
                flash_addr = (char *)SEG_D;
                flash_erase(flash_addr);
                segment_wr(flash_addr, flash_data);

                flash_assemble_segC(flash_data, &channel, panid, unicast_addr, sample_period);
                flash_addr = (char *)SEG_C;
                flash_erase(flash_addr);
                segment_wr(flash_addr, flash_data);
            }

	        //P3OUT |= 0x40; // UART Rx disable
	        //transmitreq(tx_data, j, origin_addr);
	        i = assemble_txreq(origin_addr, tx_data, j, payload);
	        uarttx_xbee(payload, i);
	        //P3OUT &= 0xbf; // UART Rx enable

	        //state = S_DEBUG;
	        state = S_DQRES4;

	        break;

	    /* State: Debug Transmit Query parameter response - Transmit status */
	    case S_DQRES4:

	        /*if (rxheader_flag == 0){
	            parse_header();
	        }else{

	            j = rx_txstat(&rxheader_flag, &rxctr, &rxpsize, rxbuf, &sensetx_fail, &sensetx);

	            if (j == 1){
	                state = S_DEBUG;
	            }else if (j != 0){
	                state = S_DQRES3; // re-transmit Query response
	            }
	        }

	        */
	        /*j = parse_header();
	        if (rxheader_flag == 1){*/
	        if (parse_header()){
	            j = rx_txstat(&rxheader_flag, &rxctr, &rxpsize, rxbuf, &sensetx_fail, &sensetx);
                if (j == 1){
                    state = S_DEBUG;
                }else if (j != 0){
                    state = S_DQRES3; // re-transmit Query response
                }
	        }

	        break;

	    /* State: Timed sleep */
	    case S_SLEEP1:
	        if (timer_flag > sleep_time ){ //exit sleep
	            state = S_DEBUG;
	            P3OUT &= 0x7f; // low to wakeup XBee
	        break;
#endif
    	}
    }
}
}

/* Interrupts */

// UART Rx ISR
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){

	/*if (rxbuf_overflow == 0){
		if (rxctr == 0){
			if (UCA0RXBUF == 0x7e){
				rxbuf[0] = 0x7e;
				rxctr++;
			}
		}else{
			rxbuf[rxctr] = UCA0RXBUF;
			rxctr++;

			// Disable UART Tx when whole packet is received
			// -> Re-enable in main function
			if (rxctr >= (rxpsize + 4)){
				P3OUT |= 0x40;	// nRTS to 0 (UART Rx disable)
			}
		}
	}

	if (rxctr == RXBUF_MAX){
		rxbuf_overflow = 1;
	}*/

    if (rxctr < RXBUF_MAX){
        if (rxctr == 0){
            // Only start filling buffer on start delimeter 0x7e
            if (UCA0RXBUF == 0x7e){
                rxbuf[0] = 0x7e;
                rxctr = 1;
            }
        }else{
            rxbuf[rxctr] = UCA0RXBUF;
            rxctr++;
        }
    }else{
        state = S_RXBUFOFW;
    }

	IFG2 = IFG2 & 0x0A;				//this clears the interrupt flags
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0 (void) {
  timer_flag++;
#ifdef SLEEP_UC
  _BIC_SR(LPM0_EXIT); // wake up from low power mode
#endif
}

/* User-defined functions */
// Parse frame packet header (0x7e,size)
int parse_header(void){
	if (rxctr >= 3){
		rxpsize = (unsigned int) rxbuf[2];
		rxpsize += ((unsigned int) rxbuf[1]) << 8;
		rxheader_flag = 1; // done fetching header
		return 1;
	}else{
		return 0;
	}
}
