# Dynamic Keymap Design

## Design goals

Key design goals include the following:
- allow configuring keymaps dynamically
- enable changing devicetree managed settings (such as RGB underglow)
- store all settings in nonvolatile storage
- enable desktop application to communicate with the keyboard to update settings

## Key challenges

The key challenges here include the following
- all devicetree data is defined at compile time only- no well defined data structures in ROM
- each feature will read devicetree data from the header file in its own (bespoke) method
- no GUI app exists for communicating with the keyboard

## Technologies to investigate
- CBOR (can we use CBOR to encode keyboard maps as JSON)
  - Can CBOR be sent over BLE? If so how?
- Settings subsystem
  - each ZMK feature would treat the settings subsystem as a client
  - when a ZMK feature supports the dynamic configuration, it will always source it via the settings subsystem
  - on first boot of a new firmware, the settings subsystem will initialize all dynamic settings from devicetree
    - how to know what is a "new" firmware?
    - do we store all devicetree settings in the settings subsystem as "Defaults" since they will already be in ROM?


## Research Notes

### ZCBOR over BLE
MCUMGR uses ZCBOR, and can communicate over BLE for firmware updates

https://github.com/apache/mynewt-mcumgr/blob/master/transport/smp-bluetooth.md
- SMP requests are sent using GATT Write and GATT Write without response
- SMP requests and response can be fragmented across multiple packets
  - Bluetooth guarantees in order delivery of packets, so reconstruction is possible
- Computer would be GATT server, device is GATT client
- BLE has official GATT services with UUIDs
  - each service can have characteristics
  - IE there is a heart rate service, with a few characteristics within
- SMP server has one characteristic, the SMP Characteristic
  - For any characteristic, we can implement things like:
    - Name
    - Description
    - Read
    - Write
    - WriteWithoutResponse
    - SignedWrite
    - Notify
    - Indicate
    - WriteableAuxiliaries
    - Broadcast
    - ExtendedProperties
- *UNVERIFIED*- I expect that MCUMgr sends CBOR data via the Write/Read implementations for the SMP characteristic
- Summary- it seems like CBOR data can be sent via BLE

### ZCBOR over USB
We can use CDC ACM here to send data over serial link.
MCUMGR SMP sample uses this.
