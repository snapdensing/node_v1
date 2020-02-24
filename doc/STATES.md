# State Machine Description

List of states:

- [`S_DEBUG`](#S_DEBUG)
- [`S_DQRES1`](#S_DQRES1)
- [`S_DQRES2`](#S_DQRES2)
- [`S_DQRES3`](#S_DQRES3)
- [`S_DQRES4`](#S_DQRES4)
- [`S_DATRES`](#S_DATRES)

<a name='S_DEBUG'></a>
## S_DEBUG: Standby/Debug
Loop state where the node waits for remote command sent through XBee.

When changing MCU parameters, the node implements the change without transitioning to another state.

<a name='S_DQRES1'></a>
## S_DQRES1: Query Parameter Response 1
Parses the command packet for the parameter we wish to query then sends the appropriate AT Command Request frame to the XBee. The MCU enters this state only when XBee radio parameters are queried.
Unconditionally transitions to state [`S_DQRES2`](#S_DQRES2).

<a name='S_DQRES2'></a>
## S_DQRES2: Query Parameter Response 2
Catches the response for the AT Command Request made in [`S_DQRES1`](#S_DQRES1). Starts assembly of queried parameter (XBee parameter) in `txbuf`. Transitions to state [`S_DQRES3`](#S_DQRES3) unless parameter is unsupported or a different packet was received through UART from the XBee, in which case it defaults back to state [`S_DEBUG`](#S_DEBUG). 

<a name='S_DQRES3'></a>
## S_DQRES3: Query Parameter Response 3
Transmits the query parameter response packet back to the requesting node. For MCU parameters, the MCU transitions directly to this state from [`S_DEBUG`](#S_DEBUG) instead of going through [`S_DQRES1`](#S_DQRES1) and [`S_DQRES2`](#S_DQRES2). Additionally, if the query was actually a write to NVM, all MCU parameters are also saved into MCU flash. This state unconditionally transitions to state [`S_DQRES4`](#S_DQRES4).

<a name='S_DQRES4'></a>
## S_DQRES4: Query Parameter Response 4
Catches the transmit request response made in [`S_DQRES3`](#S_DQRES3). Transitions to state [`S_DEBUG`](#S_DEBUG) unless the response is invalid, in which case the MCU retransmits the query parameter response by going back to [`S_DQRES3`](#S_DQRES3).

<a name='S_DATRES'></a>
## S_DATRES: Change AT Parameter Response
Catches the response for the AT Command Request made in [`S_DEBUG`](#S_DEBUG) when changing XBee AT Parameters. Loops into this state indefinitely until appropriate response is received, after which the node reverts back to state [`S_DEBUG`](#S_DEBUG).

# Modification Notes

## Adding AT parameter support for queries/set
When adding new AT commands for query/set, you will need to edit the following files/functions:

- `defines.h`: Add parameter encoding (`PARAM_XX`) and corresponding debug parse return values (`CHGXX, QUEXX`).
- `parameter_to_str()` (in parse_routines.c): Lookup table for translation from parameter encoding to AT parameter string.
- `parse_debugpacket()` (in parse_routines.c): Add corresponding return value to option `CHGXX` and `QUEXX`.
- `parse_atres()` (in parse_routines.c): Add AT command and parsed parameter (if query).
- `main()`, state `S_DEBUG`: Add options `CHGXX` and `QUEXX`.
- `main()`, state `S_DQRES2`: Add AT parameter.