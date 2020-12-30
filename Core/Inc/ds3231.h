/*
 * ds3231.h
 *
 *  Created on: Dec 28, 2020
 *      Author: johny
 */

#ifndef INC_DS3231_H_
#define INC_DS3231_H_
#include "i2c.h"

//#define DS3231_ADDRESS 0x68 // I2C address not shifted
#define DS3231_ADDRESS 0xD0

float getTemp (void);
void forceTempConv (void);

#endif /* INC_DS3231_H_ */
