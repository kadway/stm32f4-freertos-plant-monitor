/*
 * ds3231.c
 *
 *  Created on: Dec 28, 2020
 *      Author: johny
 */
#include "ds3231.h"

float getTemp (void){
	uint8_t temp[2];
	HAL_I2C_Mem_Read(&hi2c3, DS3231_ADDRESS, 0x11, 1, temp, 2, 1000);
	return ((temp[0])+(temp[1]>>6)/4.0);
	//return temp[0];
}


void forceTempConv (void){
	uint8_t status=0;
	uint8_t control=0;
	HAL_I2C_Mem_Read(&hi2c3, DS3231_ADDRESS, 0x0F, 1, &status, 1, 100);  // read status register
	if (!(status&0x04))  // if the BSY bit is not set
	{
		HAL_I2C_Mem_Read(&hi2c3, DS3231_ADDRESS, 0x0E, 1, &control, 1, 100);  // read control register
		HAL_I2C_Mem_Write(&hi2c3, DS3231_ADDRESS, 0x0E, 1, (uint8_t *)(control|(0x20)), 1, 100);  // write modified control register with CONV bit as'1'
	}
}
