# RESE2NSE Node version 1 Commands

This section lists the supported remote commands for configuration and operation of the node.

- Query commands
    - [Power level](#query_pl)
    - Aggregator address
    - Sampling period
    - Statistics
    - Control flag
    - Firmware version
- Configure commands
    - Transmit period
    - [Power level](#config_pl)
    - Radio channel
    - Aggregator address
    - Commit radio settings to NVM
    - Node Info: Node ID
    - Node Info: Node Location
    - Control flag
- Operation commands
    - Start Sensing
    - Stop Sensing
    - Timed Sleep
    - Debug Unicast
    - Debug Broadcast

<a name="query_pl"></a>
# Query Power Level
Queries the XBee power level of remote node. Node must be in standby state.

Packet format: `'QP' (0x5150)`

Node returns: `'QP'[Power Level] (0x5150[Power Level])`

- `[Power Level]` is a 1-byte value corresponding to the XBee power level. Range of values is from 0x00 to 0x04.

<a name="config_pl"></a>
# Change Power Level
Changes the XBee power level of remote node. Node must be in standby state.

Packet format: `'DP'[Power Level] (0x5150[Power Level])`

- `[Power level]` is a 1-byte value corresponding to the XBee power level. Allowable values are from 0x00 to 0x04.