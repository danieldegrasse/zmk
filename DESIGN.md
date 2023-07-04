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
- Settings subsystem
  - each ZMK feature would treat the settings subsystem as a client
  - when a ZMK feature supports the dynamic configuration, it will always source it via the settings subsystem
  - on first boot of a new firmware, the settings subsystem will initialize all dynamic settings from devicetree
    - how to know what is a "new" firmware?
    - do we store all devicetree settings in the settings subsystem as "Defaults" since they will already be in ROM?
- USB HID
  - Can we expose the keyboard data over HID?


## Research Notes

## USB HID
HID supports feature reports, which would likely allow us to read data about
the keyboard (such as the keymap) from the USB endpoint. This might
be the best solution for reading USB keyboard data, since it does
not require an additional driver

HIDAPI seems to be the library of choice for communicating with custom HID
devices, so all we need is to support a vendor defined USB HID endpoint.
