#ifndef _GPS_H_
#define _GPS_H_

/**************************************************************************/
/*!
  gps.h
  GPS Module (Neo 6M) related function definitions for use with the TM4C1294XL Board (TIVA)

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GPS_UART 0x4000E000  //UART2_BASE, pins PA6 and PA7

void GPS_setup_UART(uint32_t g_ui32SysClock,int baud);
char* GPS_Read_UART(void);
char* GPS_get_info(char *rawString);
char* GPS_parse_coordinate(char* c);
#endif