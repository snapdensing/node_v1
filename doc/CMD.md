# RESE2NSE Node version 1 Commands

This section lists the supported remote commands for configuration and operation of the node.

- Query commands
    - [Power level](#query_pl)
    - [Aggregator address](#query_aggre)
    - [Sampling period](#query_per)
    - [Statistics](#query_stat)
    - [Control flag](#query_flag)
    - Firmware version
- Configure commands
    - [Sampling period](#config_per)
    - [Power level](#config_pl)
    - [Radio channel](#config_ch)
    - [Aggregator address](#config_aggre)
    - Commit radio settings to NVM
    - Node Info: Node ID
    - Node Info: Node Location
    - [Control flag](#config_flag)
- Operation commands
    - Start Sensing
    - Stop Sensing
    - Timed Sleep
    - Debug Unicast
    - Debug Broadcast

<a name="query_aggre"></a>
# Query Aggregator address
Queries the Aggregator (sink) address of where the remote node must send its sensing data. Node must be in standby state.

Packet format: `'QA' (0x5141)`

Node returns: `'QA'[Aggregator address] (0x5141[Aggregator address])`

- `[Aggregator address]` is an 8-byte value of the aggregator's address.

<a name="query_flag"></a>
# Query Control Flag
Queries the MCU control flag (`ctrl_flag`). Node must be in standby state.

Packet format: `'QF' (0x5146)`

Node returns: `'QF[Flag]' (0x5146[Flag])`

- `[Flag]` is a 1-byte value of the MCU control flag. Bit fields are described below:
    - `Bit 7` specifies what state the node enters after boot-up. If `Bit 7` is `0`, the node enters the Sense state. Otherwise, it enters the Standby/Debug state.

<a name="query_pl"></a>
# Query Power Level
Queries the XBee power level of remote node. Node must be in standby state.

Packet format: `'QPL' (0x5150)`

Node returns: `'QPL'[Power Level] (0x5150[Power Level])`

- `[Power Level]` is a 1-byte value corresponding to the XBee power level. Range of values is from 0x00 to 0x04.

<a name="query_per"></a>
# Query Sampling Period
Queries the sampling period of remote node. Node must be in standby state.

Packet format: `'QT' (0x5154)`

Node returns: `'QT[Sampling period] (0x5154[Sampling Period])`

- `[Sampling Period]` is a 1 or 2-byte unsigned integer corresponding to the sampling period.

<a name="query_stat"></a>
# Query Statistics
Queries transmit statistics from remote node. Node must be in standby state.

Packet format: `'QS' (0x5153)`

Node returns: `'QS'[SenseTx][SenseTxFail] (0x5153[SenseTx][SenseTxFail])`

- `[SenseTx]` is a 2 byte unsigned integer counter for the number of tranmissions made. It is incremented whenever the `rx_txstat()` function is called during sensing and during a stop command. This counter is reset to 0 whenever a start command is issued.
- `[SenseTxFail]` is a 2 byte unsigned integer counter for the number of failed tranmissions. It is incremented whenever the `rx_txstat()` function is called during sensing and during a stop command. This counter is reset to 0 whenever a start command is issued.

<a name="config_aggre"></a>
# Change Aggregator address
Changes the Aggregator (sink) address of where the remote node must send its sensing data. Node must be in standby state.

Packet format: `'DA'[Aggregator address] (0x4441[Aggregator address])`

- `[Aggregator address]` is an 8-byte value of the aggregator's address.

<a name="config_flag"></a>
# Change Control Flag
Changes the MCU control flag (`ctrl_flag`). Node must be in standby state.

Packet format: `'DF[Flag]' (0x4446[Flag])`

- `[Flag]` is a 1-byte value of the MCU control flag. Bit fields are described below:
    - `Bit 7` specifies what state the node enters after boot-up. If `Bit 7` is `0`, the node enters the Sense state. Otherwise, it enters the Standby/Debug state.

<a name="config_pl"></a>
# Change Power Level
Changes the XBee power level of remote node. Node must be in standby state.

Packet format: `'DPL'[Power Level] (0x44504C[Power Level])`

- `[Power level]` is a 1-byte value corresponding to the XBee power level. Allowable values are from 0x00 to 0x04.

<a name="config_ch"></a>
# Change Radio channel
Changes the XBee channel of remote node. Node must be in standby state.

Packet format: `'DCH'[Channel] (0x444348[Channel])`

- `[Channel]` is a 1-byte value corresponding to the XBee channel. Allowable values are from 0x0C to 0x1A.

<a name="config_per"></a>
# Change Sampling Period
Changes the sampling period of remote node. Node must be in standby state.

Packet format: `'DT[Sampling period]' (0x4454[Sampling period])`

- `[Sampling Period]` is a 1-byte unsigned integer corresponding to the sampling period. (2-byte support is in the works)