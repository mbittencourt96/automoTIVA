#ifndef _GSM_H_
#define _GSM_H_

/**************************************************************************/
/*!
  gsm.h
  GSM Module (SIM800C) related function definitions for use with the TM4C1294XL Board (TIVA)

  License: TBD

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GSM_UART 0x40011000  //UART5_BASE, pins PC4 and PC5

void GSM_setup_UART(uint32_t g_ui32SysClock,int baud);
char* GSM_Read_UART(void);
void GSM_Send_Command_UART(char* command, int count);
void GSM_UARTClear(void);
char* GSM_getResponse(char* command, int delay,char* expectedResponse);
char* GSM_getResponseFromServer(char* command);
bool GSM_Config_Module(uint32_t g_ui32SysClock);
bool GSM_resetModule(void);
bool GSM_initGPRS(void);
bool GSM_initHTTP(void);
bool GSM_sendData(void);
bool GSM_finishHTTP(void);
#endif