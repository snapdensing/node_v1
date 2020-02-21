#include <msp430.h> 
#include "defines.h"
#include "defines_id.h"

/* Function Prototypes */

int parse_header(void);

//int parse_ack(char *packet, unsigned int length, char *base_addr);
//int parse_start(char *packet, unsigned int length, unsigned int *sample_period);
int parse_stop(char *packet, unsigned int length, char *origin);

//unsigned int parse_debugpacket(char *packet, unsigned int length, unsigned int *num);
unsigned int parse_debugpacket(char *packet, unsigned int length, unsigned int *num, int *parameter);
void parse_setaddr(char *packet, char *address);
void parse_srcaddr(char *packet, char *address);
int parse_atcom_query(char *packet, unsigned int length, int parameter, char *parsedparam);
unsigned int parse_setnodeid(char *packet, char *node_id);
unsigned int set_parsedparam(int chgparam, unsigned int paramval, char *parsedparam);
void parameter_to_str(int parameter, char *atcom);

#ifdef SENSOR_BATT
//int buildSense(char *tx_data, unsigned int sensor_flag, unsigned int tx_count, unsigned int *batt_out);
int buildSense(char *txbuf, unsigned int sensor_flag, unsigned int tx_count, unsigned int *batt_out);
unsigned int Battery_supply_nonreg(void);
void batt_charge(int *charge_flagp, unsigned int batt, unsigned int batt_lo, unsigned int batt_hi);
#else
//int buildSense(char *tx_data, unsigned int sensor_flag, unsigned int tx_count);
int buildSense(char *txbuf, unsigned int sensor_flag, unsigned int tx_count);
#endif

//void transmitreq(char *tx_data, int tx_data_len, char *dest_addr, char *txbuf);
void transmitreq(int tx_data_len, char *dest_addr, char *txbuf);
//void atcom(char com0, char com1, char *paramvalue, int paramlen, char *txbuf);
void atcom(int parameter, char *paramvalue, int paramlen, char *txbuf);
void append_nodeinfo(char *txbuf, unsigned int *txbuf_i, char *node_id, unsigned int node_id_len, char *node_loc, unsigned int node_loc_len);

void detect_sensor(unsigned int *sensor_flagp);

void flash_erase(char *addr);
void segment_wr(char *base_addr, char *data);
void flash_assemble_segD(char *data, char *node_id, unsigned int node_id_len, char *node_loc, unsigned int node_loc_len);
void read_segD(char *node_id, unsigned int *node_id_lenp, char *node_loc, unsigned int *node_loc_lenp);
void flash_assemble_segC(char *data, char *channel, char *panid, char *aggre, unsigned int sampling, char *ctrl_flag);
void read_segC(char *validp, char *panid, char *channel, char *aggre, unsigned int *samplingp, char *ctrl_flag);

void rst_rxbuf(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p);
int rx_txstat(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p, char *rxbuf, unsigned int *fail_ctr_p, unsigned int *tx_ctr_p);
//int rx_atres(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p, char *rxbuf, char com0, char com1, char *returndata);
int rx_atres(int *rxheader_flag_p, unsigned int *rxctr_p, unsigned int *rxpsize_p, char *rxbuf, int parameter, char *returndata);
//void param_to_atcom(int param, char *com0, char *com1);
int parse_atres(int parameter, char *returndata, char *rxbuf, unsigned int *parsedparam_len);

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

/* Main */

int main(void) {

    /** Constant Addresses **/
#ifdef DEBUGTX_ENABLE
    static char broadcast_addr[] = "\x00\x00\x00\x00\x00\x00\xff\xff"; //broadcast
#endif
    static char unicast_addr_default[]   = "\x00\x13\xA2\x00\x40\x9A\x0A\x81"; //unicast address (test)

    /** Constant messages **/
    //static char stopACK[] = "XA"; // Stop command acknowledge
    //static char startACK[] = "SA"; // Start command acknowledge

	/* Global Variables */

	/** Local counters **/
	unsigned int i,j;

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
	//char tx_data[MAXDATA]; // Data buffer, data to transmit
	char txbuf[MAXPAYLOAD]; // Transmit buffer, payload only (w/o headers and checksum)
	unsigned int tx_count;
	unsigned int temp_uint;
	char atres_status;
	int parameter;
	char parsedparam[8];
	unsigned int parsedparam_len;
	char channel; // XBee channel
	char panid[2]; // XBee ID
	char atcom_str[2];

	unsigned int standby_sample = STANDBY_SAMPLEBATT; // Standby (Debug state) battery sample period

	/** Flash **/
	char flash_data[64];
	char *flash_addr;
	char valid_segC;
    //unsigned int test_id_len;
    //unsigned int test_loc_len;
    //char test_id[31], test_loc[31];


	/** Configuration and flags **/
	unsigned int sample_period;
	int stop_flag;
	unsigned int sensor_flag;
	// Control flag (enabled if 0)
	// [7] - autostart sensing on boot
	char ctrl_flag;

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
    temp_uint = 0;

    /* Default sampling period
     * - possible overwrite by flash if value exists
     */
    sample_period = SAMPLE_PERIOD;

    /* Initialize Node ID/Loc */
    //read_segD(test_id, &test_id_len, test_loc, &test_loc_len);
    read_segD(node_id, &node_id_len, node_loc, &node_loc_len);

    // Node ID
    if (node_id_len > MAXIDLEN){
        // Use programming defaults
        node_id_len = node_id_len_default;
        for (i=0; i<node_id_len_default; i++)
            node_id[i] = node_id_default[i];
    }

    // Node Loc
    if (node_loc_len > MAXIDLEN){
        // Use programming defaults
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
    ctrl_flag = 0xff;
    read_segC(&valid_segC, panid, &channel, unicast_addr, &sample_period, &ctrl_flag);

    /** Check sensors **/
    detect_sensor(&sensor_flag);

    /* Start */

    /** Enable Interrupts **/
    __bis_SR_register(GIE);

    /** Start up delay **/
    while(timer_flag < BOOTDELAY);

    while(1){
    	switch(state){

    	/** State: Enable RTS control (AT Command Send) **/
    	case S_RTS1:

    		//P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_RTS2;
    		//atcom_enrts();
    		//atcom('D', '6', parsedparam, 0, txbuf);
    		atcom(PARAM_D6, parsedparam, 0, txbuf);

    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    		break;

    	/** State: Enable RTS control (AT Command Response) **/
    	case S_RTS2:

    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
                //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'D', '6', node_address)){
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, PARAM_D6, node_address)){
                    state = S_BOOTUP1;
                }
    		}
    		break;

    	/** State: Get Address (AT Command Send Upper) **/
    	case S_ADDR1:

    		//P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_ADDR2;
    		//atcom_shsl(1);
    		//atcom('S', 'H', parsedparam, 0, txbuf);
    		atcom(PARAM_SH, parsedparam, 0, txbuf);

    		//P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    		break;

    	/** State: Get Address (AT Command Response Upper) **/
    	case S_ADDR2:

    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
    		    //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'S', 'H', node_address)){
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, PARAM_SH, node_address)){
                    state = S_ADDR3;
                }
    		}
    		break;

    	/** State: Get Address (AT Command Send Lower) **/
    	case S_ADDR3:

    		//P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = S_ADDR4;
    		//atcom_shsl(2);
    		//atcom('S', 'L', parsedparam, 0, txbuf);
    		atcom(PARAM_SL, parsedparam, 0, txbuf);

    		//P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    		break;

    	/** State: Get Address (AT Command Response Lower) **/
    	case S_ADDR4:
    		if (rxheader_flag == 0){
    			parse_header();
    		}
    		else{
    		    //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'S', 'L', node_address)){
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, PARAM_SL, node_address)){
                    if (ctrl_flag & 0x80){ // ctrl_flag[7] == '1'
                        state = S_DEBUG;
                    }else{ // autostart
                        state = S_SENSE;
                    }
                }
    		}
    		break;

    	/** State: Bootup - set ID **/
    	case S_BOOTUP1:
    	    //P3OUT |= 0x40;  // nRTS to 1 (UART Rx disable)

    	    state = S_BOOTUP2;
    	    /* Get value from flash if it exists */
    	    if (!(valid_segC & 0x40)){
    	        temp_uint = (unsigned int)panid[0];
    	        temp_uint = (temp_uint << 8) + (unsigned int)panid[1];
    	    }else{
    	        temp_uint = XBEE_ID;
    	        panid[0] = (char)(temp_uint >> 8);
    	        panid[1] = (char)(temp_uint & 0x00ff);
    	    }
    	    //atcom_id_set(temp_uint);
    	    //atcom('I', 'D', panid, 2, txbuf);
    	    atcom(PARAM_ID, panid, 2, txbuf);

    	    //P3OUT &= 0xbf;  // nRTS to 0 (UART Rx enable)
    	    break;

    	/** State: Bootup - set ID response **/
    	case S_BOOTUP2:
            if (rxheader_flag == 0){
                parse_header();
            }
            else{
                //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'I', 'D', node_address)){
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, PARAM_ID, node_address)){
                    state = S_BOOTUP3;
                }
            }
            break;

        /** State: Bootup - set CH **/
        case S_BOOTUP3:
            //P3OUT |= 0x40;  // nRTS to 1 (UART Rx disable)

            state = S_BOOTUP4;
            /* Get value from flash if it exists */
            /*if (!(valid_segC & 0x80)){
                temp_uint = channel;
            }else{
                channel = (char)XBEE_CH;
                temp_uint = XBEE_CH;
            }*/
            if (valid_segC & 0x80){
                channel = (char)XBEE_CH;
            }
            //atcom_ch_set(temp_uint);
            //atcom('C', 'H', &channel, 1, txbuf);
            atcom(PARAM_CH, &channel, 1, txbuf);

            //P3OUT &= 0xbf;  // nRTS to 0 (UART Rx enable)
            break;

        /** State: Bootup - set CH response **/
        case S_BOOTUP4:
            if (rxheader_flag == 0){
                parse_header();
            }
            else{
                //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'C', 'H', node_address)){
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, PARAM_CH, node_address)){
                    state = S_ADDR1;
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

   			state = S_SENSERES;

   			/* Assemble Packet Data */
#ifdef SENSOR_BATT
   			//j = buildSense(tx_data,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
   			j = buildSense(txbuf,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}

   			/* Append node_id */
   			//tx_data[j] = 0x07;
   			/*txbuf[j] = 0x07;
   			j++;
   			for (i=0; i<node_id_len; i++){
   			    //tx_data[j] = node_id[i];
   			    txbuf[j] = node_id[i];
   			    j++;
   			}*/

   			/* Append node_loc */
            //tx_data[j] = ':';
   			/*txbuf[j] = ':';
            j++;
   			for (i=0; i<node_loc_len; i++){
   			    //tx_data[j] = node_loc[i];
   			    txbuf[j] = node_loc[i];
   			    j++;
   			}*/

   			append_nodeinfo(txbuf, &j, node_id, node_id_len, node_loc, node_loc_len);

#ifdef DEBUG_CHARGING
   			/* Append charge_flag to packet */
   			//tx_data[j] = (char)(charge_flag & 0x00ff);
   			txbuf[j] = (char)(charge_flag & 0x00ff);
   			j++;
#endif

#else
   			//j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
   			j = buildSense(txbuf,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
#endif

#ifdef SENSOR_BATT
            /* Charging state */
            /*if (charge_flag == 0){ // Discharging
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
            }*/

            batt_charge(&charge_flag, batt, batt_lo, batt_hi);
#endif

   			/* Transmit */
#ifdef BROADCAST
   			//transmitreq(tx_data, j, broadcast_addr, txbuf);
            transmitreq(j, broadcast_addr, txbuf);
#else
   			//transmitreq(tx_data, j, unicast_addr, txbuf);
            transmitreq(j, unicast_addr, txbuf);
#endif

   			/* Update Transmit Counter */
    		tx_count++;

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
    		        rst_rxbuf(&rxheader_flag, &rxctr, &rxpsize);
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
   			//transmitreq(stopACK, 2, origin_addr, txbuf);
    		txbuf[14] = 'X';
    		txbuf[15] = 'A';
    		transmitreq(16, origin_addr, txbuf);
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
    	    state = S_STARTRES;

    	    /* Transmit */
    	    //transmitreq(startACK, 2, origin_addr, txbuf);
    	    txbuf[14] = 'S';
    	    txbuf[15] = 'A';
    	    transmitreq(16, origin_addr, txbuf);
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


    	/** State: Debug mode entrypoint **/
    	case S_DEBUG:

#ifdef SENSOR_BATT
    		/* Standby battery monitoring */
    		if (timer_flag > standby_sample){
    			batt = Battery_supply_nonreg();
    			timer_flag = 0; // Reset timer flag
    		}

    		/*if (charge_flag == 0){ // Discharging
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
    		}*/

    		batt_charge(&charge_flag, batt, batt_lo, batt_hi);
#endif

    		/* Process packet received */
    		if (rxheader_flag == 0){
    			parse_header();

    		}else{

    			if (rxctr >= (rxpsize + 4)){ // Entire packet received

    			    temp_uint = sample_period; // Need to initialize in case command is Start retain ('S', no args)
    				j = parse_debugpacket(rxbuf, rxpsize, &temp_uint, &parameter);
    				tx_count = 0;

    				switch(j){

    					// Debug Broadcast
#ifdef DEBUGTX_ENABLE
    					case DBRD:
    						timer_flag = 0; // Reset timer flag
    						state = S_DBRD;
    						break;

    					// Debug Unicast
    					case DUNI:
    						timer_flag = 0; // Reset timer flag
    						state = S_DUNI;
    						parse_srcaddr(rxbuf,origin_addr);
    						break;
#endif

    					// Change Period
    					case CHGPER:
    					    state = S_DEBUG;
    						sample_period = temp_uint;
    						break;

    					// Change AT parameter 1 byte
    					case CHGPL:
    					case CHGCH:
    					case CHGMR:
                            //state = S_DPLRES;
    					    state = S_DATRES;
    					    parsedparam[0] = (char)(temp_uint & 0x00ff);
    					    //atcom('P', 'L', parsedparam, 1, txbuf);
    					    atcom(parameter, parsedparam, 1, txbuf);
    					    //parsedparam_len = set_parsedparam(j, temp_uint, parsedparam);
    					    //atcom(parsedparam[0], parsedparam[1], (parsedparam + 2), parsedparam_len, txbuf);
    			    		break;

    					// Change CH
    					/*case CHGCH:
    						state = S_DCHRES;
    					    channel = (char)(temp_uint & 0x00ff);
    					    atcom('C', 'H', &channel, 1, txbuf);
							break;*/

    					// Start sensing
    					case START:
    						state = S_START;
    						sample_period = temp_uint;
    						parse_srcaddr(rxbuf,origin_addr);
    						break;

    					// Change unicast address
    					case CHGSINK:
    						state = S_DEBUG;
    						parse_setaddr(rxbuf,unicast_addr);
    						break;

    					// Query XBee AT parameters
    					case QUEPL:
    					case QUEMR:
    						state = S_DQRES1;
    						/*switch (j){
    						case QUEPL:
    						    parameter = PARAM_PL; break;
    						case QUEMR:
    						    parameter = PARAM_MR; break;

    						}*/
    						parse_srcaddr(rxbuf,origin_addr);
    						break;

    					// Query unicast address
    					case QUESINK:
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    //tx_data[0] = 'Q';
    					    //tx_data[1] = 'A';
    					    txbuf[14] = 'Q';
    					    txbuf[15] = 'A';
    					    for (i=0; i<8; i++)
    					        //tx_data[i+2] = unicast_addr[i];
    					        txbuf[i+16] = unicast_addr[i];
    					    //j = 10; // tx_data length
                            j = 24; // tx_data length
    					    break;

    					// Query sampling period
    					case QUEPER:
    					//else if (j == QUEPER){
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    //tx_data[0] = 'Q';
    					    //tx_data[1] = 'T';
    					    txbuf[14] = 'Q';
    					    txbuf[15] = 'T';
    					    //tx_data[2] = (char)sample_period; //assumes period is only 8-bits
    					    if (sample_period < 256){
    					        txbuf[16] = (char)sample_period;
    					        //j = 3; // tx_data length
    					        j = 17; // tx_data length
    					    }else{
    					        txbuf[16] = (char)(sample_period >> 8);
    					        txbuf[17] = (char)(sample_period & 0x00ff);
    					        j = 18;
    					    }
    					    break;
    					//}

    					// Commit radio settings to NVM
    					case COMMIT:
    					    state = S_DQRES1;
    					    //parameter = PARAM_WR;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    break;

    					// Change node ID
    					case CHGID:
    					    state = S_DEBUG;
    					    node_id_len = parse_setnodeid(rxbuf,node_id);
    					    break;

    					// Change node loc
    					case CHGLOC:
    					    state = S_DEBUG;
    					    node_loc_len = parse_setnodeid(rxbuf,node_loc);
    					    break;

    					// Enter timed sleep
    					case SLPTIMED:
    					    state = S_SLEEP1;
    					    sleep_time = temp_uint;
    					    timer_flag = 0;
    					    P3OUT |= 0x80; // high to sleep XBee
    					    break;

    					// Query statistics
    					case QUESTAT:
    					    state = S_DQRES3;
                            parse_srcaddr(rxbuf,origin_addr);
                            //tx_data[0] = 'Q';
                            //tx_data[1] = 'S';
                            txbuf[14] = 'Q';
                            txbuf[15] = 'S';
                            // 1st parameter: sensetx
                            //tx_data[2] = (char)(sensetx >> 8);
                            //tx_data[3] = (char)(sensetx & 0x00ff);
                            txbuf[16] = (char)(sensetx >> 8);
                            txbuf[17] = (char)(sensetx & 0x00ff);
                            // 2nd parameter: sensetx_fail
                            //tx_data[4] = (char)(sensetx_fail >> 8);
                            //tx_data[5] = (char)(sensetx_fail & 0x00ff);
                            txbuf[18] = (char)(sensetx_fail >> 8);
                            txbuf[19] = (char)(sensetx_fail & 0x00ff);
                            //j = 6; // tx_data length
                            j = 20; // tx_data length
                            break;

                        // Change Autostart flag
    					case CHGFLAG:
    					    state = S_DEBUG;
    					    ctrl_flag = (char)(temp_uint & 0x00ff);
    					    break;

    					// Query Autostart flag
    					case QUEFLAG:
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    //tx_data[0] = 'Q';
    					    //tx_data[1] = 'F';
    					    txbuf[14] = 'Q';
    					    txbuf[15] = 'F';
    					    // parameter: control flag
    					    //tx_data[2] = ctrl_flag;
                            txbuf[16] = ctrl_flag;
    					    //j = 3; // tx_data length
                            j = 17; // tx_data length
    					    break;

    					// Query FW version
    					case QUEVER:
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    //tx_data[0] = 'Q';
    					    //tx_data[1] = 'V';
                            txbuf[14] = 'Q';
                            txbuf[15] = 'V';
    					    for (i=0; i<FWVERLEN; i++){
    					        //tx_data[i+2] = fwver[i];
                                txbuf[i+16] = fwver[i];
    					    }
    					    //j = 2 + FWVERLEN; // tx_data length
                            j = 16 + FWVERLEN; // tx_data length
    					    break;
    				}

    				// Reset buffer
    				rst_rxbuf(&rxheader_flag, &rxctr, &rxpsize);
    			}

    		}
    		break;

#ifdef DEBUGTX_ENABLE
    	/** State: Debug mode Broadcast **/
    	case S_DBRD:
    		if (timer_flag > sample_period){

    			P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    			/* Update Transmit Counter */
    			tx_count++;
    			if (tx_count < temp_uint){
    				state = S_DBRD;
    			}else{
    				state = S_DEBUG;
    			}

    			/* Assemble Packet Data */
#ifdef SENSOR_BATT
    			//j = buildSense(tx_data,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
    			j = buildSense(txbuf,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
#else
                //j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
                j = buildSense(txbuf,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
#endif

                /* Append node_id */
                //tx_data[j] = 0x07;
                /*txbuf[j] = 0x07;
                j++;
                for (i=0; i<node_id_len; i++){
                    //tx_data[j] = node_id[i];
                    txbuf[j] = node_id[i];
                    j++;
                }
                //tx_data[j] = ':';
                txbuf[j] = ':';
                j++;*/

                /* Append node_loc */
                /*for (i=0; i<node_loc_len; i++){
                    //tx_data[j] = node_loc[i];
                    txbuf[j] = node_loc[i];
                    j++;
                }*/

                append_nodeinfo(txbuf, &j, node_id, node_id_len, node_loc, node_loc_len);

    			/* Transmit */
    			//transmitreq(tx_data, j, broadcast_addr, txbuf);
                transmitreq(j, broadcast_addr, txbuf);

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
    		    	state = S_DUNI;
    		    }else{
    		    	state = S_DEBUG;
    		    }

    		    /* Assemble Packet Data */
#ifdef SENSOR_BATT
    		    //j = buildSense(tx_data,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
    		    j = buildSense(txbuf,sensor_flag,tx_count,&batt); //10-byte data: {'D', tx_count, 8-byte data}
#else
    		    //j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
    		    j = buildSense(txbuf,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}
#endif

                /* Append node_id */
    		    //tx_data[j] = 0x07;
                /*txbuf[j] = 0x07;
    		    j++;
                for (i=0; i<node_id_len; i++){
                    //tx_data[j] = node_id[i];
                    txbuf[j] = node_id[i];
                    j++;
                }
                //tx_data[j] = ':';
                txbuf[j] = ':';
                j++;*/

                /* Append node_loc */
                /*for (i=0; i<node_loc_len; i++){
                    //tx_data[j] = node_loc[i];
                    txbuf[j] = node_loc[i];
                    j++;
                }*/

                append_nodeinfo(txbuf, &j, node_id, node_id_len, node_loc, node_loc_len);

    		    /* Transmit */
    		    //transmitreq(tx_data, j, origin_addr, txbuf);
                transmitreq(j, origin_addr, txbuf);

    		    /* Reset Timer flag */
    		    timer_flag = 0;

    		    P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    		}
    		break;
#endif

    	/* State: Debug wait for PL AT command response */
    	//case S_DPLRES:
        case S_DATRES:

    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
                //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'P', 'L', &atres_status)){
    		    //parameter_to_str(parameter, parsedparam);
    		    //if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, parsedparam[0], parsedparam[1], &atres_status)){
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, parameter, &atres_status)){
                    state = S_DEBUG;
                }
    		}
    		break;

		/* State: Debug wait for CH AT command response */
		/*case S_DCHRES:

			if (rxheader_flag == 0){
				parse_header();
			}else{
                if (rx_atres(&rxheader_flag, &rxctr, &rxpsize, rxbuf, 'C', 'H', &atres_status)){
                    state = S_DEBUG;
                }
			}
			break;*/

		/* State: Debug Query parameter */
		case S_DQRES1:
		    //param_to_atcom(parameter, &parsedparam[0], &parsedparam[1]);
		    //atcom(parsedparam[0], parsedparam[1], &parsedparam[2], 0, txbuf);
		    atcom(parameter, parsedparam, 0, txbuf);
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

		            //j = parse_atcom_query(rxbuf, rxpsize, parameter, parsedparam); // j is parsed parameter length
		            if (parse_atres(parameter, parsedparam, rxbuf, &parsedparam_len)){

		            //if (j > 0){
                        txbuf[14] = 'Q';

                        /* Set txbuf[15:16] */
		                parameter_to_str(parameter, atcom_str);
		                txbuf[15] = atcom_str[0];
		                txbuf[16] = atcom_str[1];

		                /* Set parsed parameter (txbuf[17:]) */
		                for (i=0; i<parsedparam_len; i++){
		                    txbuf[17+i] = parsedparam[i];
		                }
		                j = 17 + parsedparam_len;

		                /* Set next state */
		                switch(parameter){
		                case PARAM_PL:
		                case PARAM_WR:
		                case PARAM_MR:
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
		            rst_rxbuf(&rxheader_flag, &rxctr, &rxpsize);
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

                flash_assemble_segC(flash_data, &channel, panid, unicast_addr, sample_period, &ctrl_flag);
                flash_addr = (char *)SEG_C;
                flash_erase(flash_addr);
                segment_wr(flash_addr, flash_data);
            }

            //transmitreq(tx_data, j, origin_addr, txbuf);
            transmitreq(j, origin_addr, txbuf);
	        state = S_DQRES4;

	        break;

	    /* State: Debug Transmit Query parameter response - Transmit status */
	    case S_DQRES4:
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
	        }
	        break;

    	}
    }
}


/* Interrupts */

// UART Rx ISR
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){

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
