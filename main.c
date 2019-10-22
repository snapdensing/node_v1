#include <msp430.h> 
#include "defines.h"

/* Function Prototypes */

int parse_header(void);

int parse_atres(char com0, char com1, char *returndata, char *packet, int length);

int parse_ack(char *packet, int length, char *base_addr);
int parse_start(char *packet, int length, int *sample_period);
int parse_stop(char *packet, int length, char *origin);

#ifdef MODE_DEBUG
int parse_debugpacket(char *packet, int length, int *num);
void parse_setaddr(char *packet, char *address);
void parse_srcaddr(char *packet, char *address);
int parse_atcom_query(char *packet, int length, int parameter, char *parsedparam);
#endif

int buildSense(char *tx_data, unsigned int sensor_flag, int tx_count);

void atcom_shsl(int sel);
void transmitreq(char *tx_data_loc, int tx_data_len_loc, char *tx_dest_loc);
void atcom_enrts(void);
void atcom_pl_set(int val);
void atcom_ch_set(int val);
void atcom_query(int param);

int check_sensor(int sensor_id);
unsigned int detect_sensor(void);

/* Global Variables */

/** Receive Buffer **/
char rxbuf[RXBUF_MAX];	//receive shift register
int rxctr;				//received bytes counter
int rxpsize;			//received packet size
int rxheader_flag;		//receiving frame headers flag (1 when done)
int rxbuf_overflow;

/** Timer **/
int timer_flag;

/** State Encoding **/
int state;

/** Constant Addresses **/
char broadcast_addr[] = "\x00\x00\x00\x00\x00\x00\xff\xff"; //broadcast
char unicast_addr_default[]   = "\x00\x13\xA2\x00\x40\x9A\x0A\x81"; //unicast address (test)

/** Constant messages **/
char stopACK[] = "XA"; // Stop command acknowledge


/* Main */
int main(void) {

	/* Global Variables */

	/** Local counters **/
	int i,j;

	/** Address Storage **/
	char node_address[8]; // Node XBee address
	char unicast_addr[8]; // Base station address
	char origin_addr[8]; //

	/** Transmit buffer **/
	char tx_data[73];
	int tx_count;
	int checksum;
#ifdef MODE_DEBUG
	int txmax;
	char atres_status;
	int parameter = 0;
	char parsedparam[8];
#endif


	/** Configuration and flags **/
	int sample_period;
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
    txmax = 0;
#endif

    /** Check sensors **/
    sensor_flag = detect_sensor();

    /** Base station sent configuration **/
    //sample_period = 2;
    sample_period = SAMPLE_PERIOD;

    /** Initialize Default unicast address **/
    for (i=0; i<8; i++){
    	origin_addr[i] = unicast_addr_default[i];
    	unicast_addr[i] = unicast_addr_default[i];
    }

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

    				j = parse_atres('S','L',node_address,rxbuf,rxpsize);
    				if (j == 1){
#ifdef MODE_DEBUG
    					state = S_DEBUG;
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

    	/** State: Initialization - Broadcast presence to base station **/
    	case S_INIT:

    		// Transmit "Present" signal
    	    if (timer_flag > PRES_PERIOD){

    			P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    			tx_data[0] = 'P';
    		    transmitreq(tx_data, 1, broadcast_addr);
    		    timer_flag = 0;

    		    P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    	    }

    	    // Check if acknowledge signal from base station
    	    if (rxheader_flag == 0){
    	    	parse_header();
    	    }
    	    else{
    	    	if (rxctr >= (rxpsize + 4)){ // Entire packet received

    	    		j = parse_ack(rxbuf,rxpsize,unicast_addr);

    	    		if (j == 1){
    	    			state = S_READY;
    	    		}

    	    		// Reset buffer
    	    		rxctr = 0;
    	    		rxheader_flag = 0;
    	    		rxpsize = 0;
    	    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    	    	}
    	    }
    	    break;

    	/** State: Wait for base station signal to start sensing **/
    	case S_READY:
    		// Wait for start signal from base station
    		if (rxheader_flag == 0){
    			parse_header();
    		}else{
    			if (rxctr >= (rxpsize + 4)){ // Entire packet received

    				j = parse_start(rxbuf, rxpsize, &sample_period);

    				if (j == 1){
    					timer_flag = 0; // Reset timer flag
    					state = S_SENSE;
    				}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;
    				P3OUT &= 0xbf;	// nRTS to 0 (UART enable)

    			}
    		}

    		break;

    	/** State: Perform sensing and transmit **/
    	case S_SENSE:
    		/* Sense and transmit
    		 * - use constant SAMPLE_PERIOD if hardcoded period in defines.h
    		 * - use variable sample_period if base station configured period
    		 */
#ifdef SLEEP_UC
    		P3OUT |= 0x40; // nRTS to 1 (UART Rx disable)

    		/* Assemble Packet Data */
    		j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}

    		/* Transmit */
#ifdef BROADCAST
   			transmitreq(tx_data, j, broadcast_addr);
#else
   			//transmitreq(tx_data, j, base_address);
   			transmitreq(tx_data, j, unicast_addr);
#endif

   			/* Update Transmit Counter */
   			tx_count++;

   			/* Sleep XBEE */
#ifdef SLEEP_XBEE
   		    P3OUT |= 0x80; // set P3.7 output to 1 (XBEE sleep)
#endif

   			/* Reset Timer flag */
   			timer_flag = 0;

   		    /* Sleep MSP */
   		    while (timer_flag < SAMPLE_PERIOD){
   		    	_BIS_SR(LPM0_bits + GIE); // Enter LPM0 w/interrupt
   		    }

   		    /* Wake XBEE */
#ifdef SLEEP_XBEE
   		    P3OUT &= 0x7f; // set P3.7 output to 0 (XBEE awake)
#endif

#else
    		//if (timer_flag > SAMPLE_PERIOD){
   		    //if (timer_flag > sample_period){

    			P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    			//state = S_WINDOW;
    			//state = S_SENSE;
    			state = NS_SENSE;

    			/* Assemble Packet Data */
    			j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}

    			/* Transmit */
#ifdef BROADCAST
    			transmitreq(tx_data, j, broadcast_addr);
#else
    			//transmitreq(tx_data, j, base_address);
    			transmitreq(tx_data, j, unicast_addr);
#endif

    			/* Update Transmit Counter */
    			tx_count++;

    			/* Reset Timer flag */
    			timer_flag = 0;

    			P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    			/* Reset Stop flag for Stop Window state */
				stop_flag = 0;

    		//}
    		break;
#endif

    	/** State: Window for stopping node sensing **/
    	case S_WINDOW:

    		// Wait for stop signal
    		if (stop_flag == 0){
    			//while (timer_flag < STOP_PERIOD){
    			while (timer_flag < sample_period){
    				if (rxheader_flag == 0){
    					parse_header();
    				}else{
    					if (rxctr >= (rxpsize + 4)){ // Entire packet received

    						stop_flag = parse_stop(rxbuf, rxpsize, origin_addr);
    						//stop_flag = j;

    						// Reset buffer
    						rxctr = 0;
    						rxheader_flag = 0;
    						rxpsize = 0;
    						P3OUT &= 0xbf;	// nRTS to 1 (UART enable)
    					}
    				}
    			}
    		}

    		if (stop_flag == 1){
    			//state = S_DEBUG;
    			state = NS_WINBRK;
    		}else{
    			timer_flag = 0; // Reset timer flag
    			//state = S_SENSE;
    			state = NS_WINLOOP;
    		}
    		break;

    	/** State: Stop signal acknowledge **/
    	case S_STOP:
    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    		state = NS_STOP;

    		/* Assemble Packet Data */
    		//tx_data[0] = stopACK[0];
    		//tx_data[1] = stopACK[1];

    		/* Transmit */
   			transmitreq(stopACK, 2, origin_addr);

    		/* Reset Timer flag */
    		//timer_flag = 0;

    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)

    		break;

#ifdef MODE_DEBUG
    	/** State: Debug mode entrypoint **/
    	case S_DEBUG:

    		if (rxheader_flag == 0){
    			parse_header();

    		}else{

    			if (rxctr >= (rxpsize + 4)){ // Entire packet received

    				j = parse_debugpacket(rxbuf, rxpsize, &txmax);

    				if (j != 0){
    					timer_flag = 0; // Reset timer flag
    					tx_count = 0;
    					// Debug Broadcast
    					if (j == 1)
    						state = NS_DEBUG1;
    					// Debug Unicast
    					else if (j == 2){
    						state = NS_DEBUG2;
    						parse_srcaddr(rxbuf,origin_addr);
    					}
    					// Change Period
    					else if (j == 3){
    						state = NS_DEBUG3;
    						sample_period = txmax;
    					}
    					// Change PL
    					else if (j == 4){
    						state = NS_DEBUG4;
    			    		P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)
    			    		atcom_pl_set(txmax);
    			    		P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    					}
    					// Change CH
    					else if (j == 5){
    						state = NS_DEBUG5;
    						P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)
							atcom_ch_set(txmax);
							P3OUT &= 0xbf;	// nRTS to 0 (UART Rx enable)
    					}
    					// Start sensing
    					else if (j == 6){
    						state = S_SENSE;
    						sample_period = txmax;
    					}
    					// Change unicast address
    					else if (j == 7){
    						state = S_DEBUG;
    						parse_setaddr(rxbuf,unicast_addr);
    					}
    					// Query power level
    					else if (j == 8){
    						state = S_DQRES1;
    						parameter = PARAM_PL;
    						parse_srcaddr(rxbuf,origin_addr);
    					}
    					// Query unicast address
    					else if (j == 9){
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    tx_data[0] = 'Q';
    					    tx_data[1] = 'A';
    					    for (i=0; i<8; i++)
    					        tx_data[i+2] = unicast_addr[i];
    					    j = 10; // tx_data length
    					}
    					// Query sampling period
    					else if (j == 10){
    					    state = S_DQRES3;
    					    parse_srcaddr(rxbuf,origin_addr);
    					    tx_data[0] = 'Q';
    					    tx_data[1] = 'T';
    					    tx_data[2] = (char)sample_period; //assumes period is only 8-bits
    					    j = 3; // tx_data length

    					}
    				}

    				// Reset buffer
    				rxctr = 0;
    				rxheader_flag = 0;
    				rxpsize = 0;
    				P3OUT &= 0xbf; // nRTS to 1 (UART enable)
    			}

    		}
    		break;

    	/** State: Debug mode Broadcast **/
    	case S_DBRD:
    		if (timer_flag > sample_period){

    			P3OUT |= 0x40;	// nRTS to 1 (UART Rx disable)

    			/* Update Transmit Counter */
    			tx_count++;
    			if (tx_count < txmax){
    				state = NS_DBRDLOOP;
    			}else{
    				state = NS_DBRDBRK;
    			}

    			/* Assemble Packet Data */
    			j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}

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
    		    if (tx_count < txmax){
    		    	state = NS_DUNILOOP;
    		    }else{
    		    	state = NS_DUNIBRK;
    		    }

    		    /* Assemble Packet Data */
    		    j = buildSense(tx_data,sensor_flag,tx_count); //10-byte data: {'D', tx_count, 8-byte data}

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
    					state = NS_DPLRES;
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
	        P3OUT |= 0x40; // UART Rx disable
	        transmitreq(tx_data, j, origin_addr);
	        P3OUT &= 0xbf; // UART Rx enable
	        state = S_DEBUG;
	        break;
#endif
    	}
    }
}

/* Interrupts */

// UART Rx ISR
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void){

	if (rxbuf_overflow == 0){
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
		rxpsize = (int) rxbuf[2];
		rxpsize += ((int) rxbuf[1]) << 8;
		rxheader_flag = 1; // done fetching header
		return 1;
	}else{
		return 0;
	}
}
