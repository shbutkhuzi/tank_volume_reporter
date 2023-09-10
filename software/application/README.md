# Application

This is the main application code that is able to do the following:

- Download new firmware from FTP server
- Change settings and store them in EEPROM memory when requested from admin
- Respond to messages containing key word with the information currently gathered
- Send critical messages to admin such as low battery level and unexpected disconnection of solar panel

## Settings that can be modified by admin

- Enable/Disable responding to requests
- Enable/Disable temperature reporting
- Enbale/Disable volume rate of change reporting
- Enable/Disable entering standby mode during some period of the day to save battery
- Tank parameters such as: cylinder height, radius, dome height
- GPRS context settings: APN
- FTP server settings: username, password, file path, file name, local file name
- Admin numbers (4 in total)
- Network provider name
- Key words (2 in total)
- Wakeup and standby times when standby mode is enabled


Non admin users can send key word to this device, on which the device will respond with the volume, temperature(if enabled) and rate of change of volume(if enabled),
otherwise the message will be discarded and deleted.

Admin users additionallly can read configuration parameters, current voltage level of the battery, temperature readings(water temperature measured by DS18B20, as well as STM32 temperature that is roughly same as the temperature inside the box),
distance reading from the sensor, solar panel voltage, as well as presense/absense statuses of solar panel and DS18B20 sensor.

Also, only admin 0 can trigger software upgrade, which includes downloading the new firmware from FTP server, setting corresponding bit in EEPROM memory
and jumping to bootloader program to update main application code.
