# Plant watering and monitoring system (FreeRTOS in STM32F4)

This is the code repository for a hobby plant watering and monitoring system.
The hardware used is a [STM32F407VET6](https://stm32-base.org/boards/STM32F407VET6-STM32-F4VE-V2.0.html) development board with a [W25Q16](https://www.digchip.com/datasheets/parts/datasheet/523/W25Q16.php) external flash memory.
FreeRTOS is used as the operating system and initial code was generated with STM32CubeMX/IDE. External flash driver is from [nimaltd](https://github.com/nimaltd/) Thanks:)

The idea of the project is to have an easily configurable system that can be adjusted to the user specific needs.
The system generates the control signals for turning on/off water pumps and solenoid valves at specific time intervals
but it can also take soil moisture levels into consideration
and actuate only when a determined threshold is crossed.

The time and duration of watering, as well as a timestamped moisture level (ADCs voltage) 
and ambient temperature (I2C with DS3231) are saved into the external flash.
They can be retrieved through SPI commands to STM32.

The NodeMCU is used to facilitate retrieving the stored data, as well as to send new configuration settings.
The code for NodeMCU and the client script, is kept in another code repository, [check here](https://github.com/kadway/nodemcu-micropython-plant-monitor)

Several different watering areas can se configured, each with its associated moisture sensors (ADC channels), water pump and solenoid valves.

####  Usage example
![alt text](wateringsystem.jpg)


#### How to start

You should have an STM32 board with JTAG or SWD and an STM32 programmer.
If you don't use same development board you should take care of getting an external flash memory and ensuring the SPI lines used are the same or adapt the code to your configuration.
I'm using STM32CubeIDE to compile and an STLink to program the STM32.




------

#### Contributing
If you find the project interesting and would like to contribute feel free to send a pull request.
I'm also open for suggestions if you have ideas for To-Do's.

#### To do:
* Complete/extend the communication task with additional commands (configure rtc... etc)
* Consider that the flash sectors for the logged data (moisture and watering times) have to be erased in case they need to be overwritten

#### known bugs:
 * max of 5 watering areas can be stored due to incorrect offset calculation when saving the data structures to the external flash.
 * SPI command for clearing the logged data to the flash is corrupting the configuration data,
    work around is to read all saved data and reset to default configuration (erases the external flash completely before writing defaults)