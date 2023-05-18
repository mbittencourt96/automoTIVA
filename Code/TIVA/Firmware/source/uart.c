#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_can.h"
#include "inc/hw_ints.h"
#include "driverlib/can.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

uint8_t ui8DataRx[100];

//UART related function declarations

void setupUART7(uint32_t g_ui32SysClock, int baud)
{ 
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART7) && !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC))
    {
    }                                                 //Wait until UART7 and PORT C is ready
    
   
    GPIOPinConfigure(GPIO_PC4_U7RX);
    GPIOPinConfigure(GPIO_PC5_U7TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    UARTConfigSetExpClk(UART7_BASE, g_ui32SysClock, baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    
    UARTEnable(UART7_BASE); 
}

void setupUART0(uint32_t g_ui32SysClock, int baud)
{ 
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0) && !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
    {
    }                                                 //Wait until UART0 and PORT A is ready
    
   
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
    
    UARTEnable(UART0_BASE); 
}


void UARTSend(uint32_t baseUART,char* pui8Buffer, int count)
{
    //
    // Loop while there are more characters to send.
    //
    while(count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPut(baseUART, *pui8Buffer++);
    }
    
}

char* UARTRead(uint32_t baseUART,char* outputUARTString) 
{
    char caract;
    int i = 0;
    
    if (UARTCharsAvail(baseUART))
    {
    do
     {
        caract = (unsigned char) UARTCharGet(baseUART);
        
        if (caract != '\r')
        {
          outputUARTString[i++] = caract;
    }
        
      }while(UARTCharsAvail(baseUART));
      
      if (strlen(outputUARTString) < 16)
      {
        outputUARTString[i++] = '\r';
      }
      
      outputUARTString[i] = '\0';
     
      char* returnString = outputUARTString;
      
      free(outputUARTString);
      
      return returnString;
    }
    else
    {
      return " ";
    }
    
}

void UARTReadThenSend(uint32_t UART_to_read, uint32_t UART_to_send) 
{
     while(UARTCharsAvail(UART_to_read))
     {
        UARTCharPut(UART_to_send,(unsigned char) UARTCharGetNonBlocking(UART_to_read));
     }  
}

void UARTClear(uint32_t baseUART)
{
  UARTDisable(baseUART);
  
  while(UARTCharsAvail(baseUART))
  {
      UARTCharGetNonBlocking(baseUART);
      
  }
  
  UARTFIFODisable(baseUART);
 
  UARTEnable(baseUART);
  UARTFIFOEnable(baseUART);
}
