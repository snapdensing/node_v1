# RESE2NSE Node version 1 Commands

This section lists the supported remote commands for configuration and operation of the node.

- Query commands
    - [Aggregator address](#query_aggre)
    - [Sampling period](#query_per)
    - [Statistics](#query_stat)
    - [Control flag](#query_flag)
    - Firmware version
    - XBee AT Parameters:
        - [Mesh Unicast Retries (MR)](#query_mr)
        - [Network Hops (NH)](#query_nh)
        - [Power level (PL)](#query_pl)
- Configure commands
    - [Sampling period](#config_per)
    - [Aggregator address](#config_aggre)
    - Commit radio settings to NVM
    - Node Info: Node ID
    - Node Info: Node Location
    - [Control flag](#config_flag)
    - XBee AT Parameters:
        - [Radio channel (CH)](#config_ch)
        - [Mesh Unicast Retries (MR)](#config_mr)
        - [Network Hops (NH)](#config_nh)
        - [Power level (PL)](#config_pl)
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

<a name="query_mr"></a>
# Query Mesh Unicast Retries
Queries the XBee maximum unicast retries of remote node. Node must be in standby state.

Packet format: `'QMR' (0x514D52)`

Node returns: `'QMR'[Max Retries] (0x514D52[Max Retries])`

- `[Max Retries]` is a 1-byte value corresponding to the XBee maximum unicast retries. Range of values is from 0x00 to 0x07.

<a name="query_nh"></a>
# Query Network Hops
Queries the XBee maximum network hops of remote node. Node must be in standby state.

Packet format: `'QNH' (0x514E48)`

Node returns: `'QNH'[Max Hops] (0x514E48[Max Hops])`

- `[Max Hops]` is a 1-byte value corresponding to the XBee maximum unicast retries. Range of values is from 0x01 to 0x20.

<a name="query_pl"></a>
# Query Power Level
Queries the XBee power level of remote node. Node must be in standby state.

Packet format: `'QPL' (0x51504C)`

Node returns: `'QPL'[Power Level] (0x51504C[Power Level])`

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

<a name="config_mr"></a>
# Change Mesh Unicast Retries
Changes the XBee maximum unicast retries of remote node. Node must be in standby state.

Packet format: `'DMR'[Max Retries] (0x444D52[Max Retries])`

- `[Max Retries]` is a 1-byte value corresponding to the XBee maximum unicast retries. Range of values is from 0x00 to 0x07.

<a name="config_nh"></a>
# Change Network Hops
Changes the XBee maximum network hops of remote node. Node must be in standby state.

Packet format: `'DNH[Max Hops]' (0x514E48[Max Hops])`

- `[Max Hops]` is a 1-byte value corresponding to the XBee maximum unicast retries. Range of values is from 0x01 to 0x20.

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