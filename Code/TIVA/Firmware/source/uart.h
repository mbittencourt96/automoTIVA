
#ifndef _UART_H_
#define _UART_H_

/**************************************************************************/
/*!
  uart.h
  UART related function definitions for the TM4C1294XL Board (TIVA)

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

void setupUART6(uint32_t g_ui32SysClock,int baud);
void setupUART7(uint32_t g_ui32SysClock, int baud);
void setupUART5(uint32_t g_ui32SysClock, int baud);
void setupUART0(uint32_t g_ui32SysClock, int baud);
void UARTReadThenSend(uint32_t UART_to_read, uint32_t UART_to_send);
void UARTSend(uint32_t baseUART,char* pui8Buffer, int count);
char* UARTRead(uint32_t baseUART);
void UARTClear(uint32_t baseUART);
#endif