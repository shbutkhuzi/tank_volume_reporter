# Bootloader

This code occupies first 28K bytes of STM32 flash memory and the only purpose it has is to update main application code.

Bootloader first reads configuration bit from EEPROM memory and if condition is satisfied it starts software upgrade, otherwise it jumps to main application code.
Jumping between applications is done as described in this [repository](https://github.com/viktorvano/STM32-Bootloader/tree/master)

Before enteing to bootloader, new software binary file is primarily saved in file system of SIM800L module, which then is read by bootloader program and is copied to flash memory starting from address `0x8007000` which is start address of main program
