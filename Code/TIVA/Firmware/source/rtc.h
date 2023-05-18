/**************************************************************************/
/*!
  RTC.h
  Adaptation of the Arduino Library RTCLib by JeeLab (https://github.com/adafruit/RTClib) for the TM4C1294XL Board and DS3231 RTC module (TIVA)

  License: TBD

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

#ifndef _RTC_H_
#define _RTC_H_

/* Constants */

#define RTC_ADDRESS 0x68   // I2C address for DS3231
#define RTC_STATUSREG 0x0F // Status register of DS3231
#define NUM_OF_I2CBYTES   1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
    
/* I2C initialization function for RTC Module */
void RTC_begin_I2C(uint32_t g_ui32SysClock);

/* Adjust time to RTC module */
void RTC_adjust_time(uint16_t year, uint8_t month, uint8_t dow, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

/*Returns current datetime as a string */
char* RTC_now(void);

/*Write byte to I2C Slave (RTC module)*/
void RTC_write_I2C(uint8_t b,uint8_t reg);

/*Read byte from I2C Slave (RTC module)*/
void RTC_read_I2C(uint8_t reg);

/*Interrupt handler for I2C*/
void I2CIntHandler_FIFO(void);

/*Convert binary to BCD value */
uint8_t bin2bcd(uint8_t val);

/*Convert BCD to binary value */
uint8_t bcd2bin(uint8_t val);

/*Get Day of week */
uint8_t dowToDS3231(uint8_t d);

#endif
