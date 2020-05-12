# Plant watering and monitor (stm32)
This is a project repository for a hobby plant watering and monitoring system.  
The hardware is a STM32F407VET6 development board with a W25Q16 external flash memory.  
FreeRTOS is used as the operating system and initial code was generated with STM32CubeMX/IDE.  
Communication through wifi for reading data or changing configurations is done via a nodemcu (esp8266) with micropython firmware, which communicates with the stm32 via SPI.  
Python code for nodemcu and pc client is in this repository: https://github.com/kadway/plant-monitor-nodemcu-wifi-bridge   
  
Thanks to [nimaltd](https://github.com/nimaltd/) for the W25Q16 driver code.  

#### To do:
* Detailed README
* Add RTC time to ADC measurements and actuation log
* Add ambient temperature measurement - will possibly use DS3232 with i2c communication
* Assign GPIOs for the actuation of the water pumps and solenoid valves
* Complete/extend the communication task with additional commands (configure rtc... etc)
* Consider that the flash sectors have to be erased in case they need to be overwritten
