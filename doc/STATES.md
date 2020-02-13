# State Machine Description

List of states:

- [`S_DEBUG`](S_DEBUG)
- [`S_DQRES1`](S_DQRES1)
- [`S_DQRES2`](S_DQRES2)
- [`S_DQRES3`](S_DQRES3)
- [`S_DQRES4`](S_DQRES4)

<a name='S_DEBUG'></a>
## S_DEBUG: Standby/Debug

<a name='S_DQRES1'></a>
## S_DQRES1: Query Parameter Response 1
Parses the command packet for the parameter we wish to query then sends the appropriate AT Command Request frame to the XBee. The MCU enters this state only when XBee radio parameters are queried.
Unconditionally transitions to state [`S_DQRES2`](S_DQRES2).

<a name='S_DQRES2'></a>
## S_DQRES2: Query Parameter Response 2
Catches the response for the AT Command Request made in [`S_DQRES1`](S_DQRES1). Starts assembly of queried parameter (XBee parameter) in `txbuf`. Transitions to state [`S_DQRES3`](S_DQRES3) unless parameter is unsupported or a different packet was received through UART from the XBee, in which case it defaults back to state [`S_DEBUG`](S_DEBUG). 

<a name='S_DQRES3'></a>
## S_DQRES3: Query Parameter Response 3
Transmits the query parameter response packet back to the requesting node. For MCU parameters, the MCU transitions directly to this state from [`S_DEBUG`](S_DEBUG) instead of going through [`S_DQRES1`](S_DQRES1) and [`S_DQRES2`](S_DQRES2). Additionally, if the query was actually a write to NVM, all MCU parameters are also saved into MCU flash. This state unconditionally transitions to state [`S_DQRES4`](S_DQRES4).

<a name='S_DQRES4'></a>
## S_DQRES4: Query Parameter Response 4
Catches the transmit request response made in [`S_DQRES3`](S_DQRES3). Transitions to state [`S_DEBUG`](S_DEBUG) unless the response is invalid, in which case the MCU retransmits the query parameter response by going back to [`S_DQRES3`](S_DQRES3).