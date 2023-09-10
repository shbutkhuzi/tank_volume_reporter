# Bootloader

This code occupies first 28K bytes of STM32 flash memory and the only purpose it has is to update main application code.

Bootloader first reads configuration bit from EEPROM memory and if condition is satisfied it starts software upgrade, otherwise it jumps to main application code.
Jumping to main application is similar as it is described in this [repository](https://github.com/viktorvano/STM32-Bootloader/tree/master)
