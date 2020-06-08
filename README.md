# RESE2NSE Node version 1 firmware

This repository contains the firmware deployed in the version 1 (off-the-shelf components) node of the RESE2NSE project. Node hardware consists of an MSP430 microcontroller interfaced to a Digimesh XBee 2.4GHz radio.

- [Configuration commands](doc/CMD.md)

## Packet format

Once the node starts sensing, it will send N-byte packets (payload) in the following format. All characters are encoded in ASCII.

- byte 0: 'D'
- byte 1: counter
- bytes 2 to N-1: sensor data (variable)
    - [0x00][2 bytes] - unregulated battery voltage
    - [0x01][2 bytes] - MSP internal temperature
    - [0x02][2 bytes] - DHT11 temperature
    - [0x03][2 bytes] - DHT11 humidity
    - [0x04][2 bytes] - DHT22 temperature
    - [0x05][2 bytes] - DHT22 humidity
    - [0x06][16 bytes] - Current sensor
    - [0x07][Node ID]':'[Node loc] - Sensor identifier
