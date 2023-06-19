/**************************************************************************/
/*!
  gsm.c (Source)

  Implementation of GSM-related functions (using SIM800C module) for the TM4C1294XL Board (TIVA)

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "gsm.h"
#include "uart.h"
#include "timer_systick.h"

#include "driverlib/i2c.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "inc/hw_udma.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "utils/random.h"
#include "utils/uartstdio.h"

void GSM_setup_UART(uint32_t g_ui32SysClock,int baud)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART5) && !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC))
  {
  }                                                 //Wait until UART6 and PORT P is ready
  
  GPIOPinConfigure(GPIO_PC6_U5RX);
  GPIOPinConfigure(GPIO_PC7_U5TX);
  GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  UARTConfigSetExpClk(GSM_UART, g_ui32SysClock, baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  
  UARTEnable(GSM_UART);
  
  setupUART0(g_ui32SysClock,9600);
}

char* GSM_Read_UART(void)
{
  char caract;
  int i = 0;
  char* outputString = (char*) malloc(100*sizeof(char));
  
   if (UARTCharsAvail(GSM_UART))
   {
    do
   {
      caract = (unsigned char) UARTCharGet(GSM_UART);
      
      if (caract != '\r')
      {
        outputString[i++] = caract;
      } 
      
   } while(UARTCharsAvail(GSM_UART));
   
    outputString[i] = '\n';
    outputString[i] = '\0';
    
   return outputString;  
   }
   
   else
   {
     return " ";
   }
}

void GSM_Send_Command_UART(char* command, int count)
{
  
  char* commandSend = command;
  //
    // Loop while there are more characters to send.
    //
    while(count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPut(GSM_UART, *commandSend++);
    }
}


void GSM_UARTClear(void)
{
  UARTDisable(GSM_UART);
  UARTFIFODisable(GSM_UART);
 
  UARTEnable(GSM_UART);
  UARTFIFOEnable(GSM_UART);

}
     
bool GSM_resetModule(void)
{
  char* response = " ";
  char* command = " "; 
  
  command  = "AT+CFUN=1,1\r";
  response = GSM_getResponse(command,1000,"OK");
  if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
  return true;
}
     
char* GSM_getResponse(char* command, int delay,char* expectedResponse)
{
  
  char* response = " ";
  int counter = 0;
  
  do
    {
      GSM_Send_Command_UART(command, strlen(command));
      delay_ms(delay);
      response = GSM_Read_UART();
      counter++;
      
      if (counter > 5)
      {
        return "Timeout";
      }
    } while (strstr(response,expectedResponse) == NULL);
  
  return response;
}

char* GSM_getResponseFromServer(char* command)
{
  
  char* response = " ";
  do
    {
      GSM_Send_Command_UART(command, strlen(command));
      response = GSM_Read_UART();
    } while (strlen(response) < 44);
  
  return response;
}


bool GSM_Config_Module(uint32_t g_ui32SysClock)
{
    char* response = " ";
    char* command = " "; 
    
    GSM_setup_UART(g_ui32SysClock,9600);
    setupUART0(g_ui32SysClock, 9600);
    setupTimer();

    delay_s(3);
  
     command  = "AT\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+CFUN=1\r";
     response = GSM_getResponse(command,1000,"OK");
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     command = "AT+CPIN?\r";
     response = GSM_getResponse(command,1000,"READY");
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+COPS?\r";
     response = GSM_getResponse(command,500,"+COPS");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
 
     command = "AT+CREG?\r";
     response = GSM_getResponse(command,500,"+CREG");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+CMEE=2\r";
     response = GSM_getResponse(command,500,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
    
    return true;
}

bool GSM_initGPRS(void)
{
     char* response = " ";
     char* command = " "; 
     
     command = "AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+SAPBR=3,1,\"APN\",\"timbrasil.br\"\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+SAPBR=3,1,\"USER\",\"tim\"\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+SAPBR=3,1,\"PWD\",\"tim\"\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+CSTT=\"timbrasil.br\",\"tim\",\"tim\"\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+SAPBR=1,1\r";
     response = GSM_getResponse(command,5000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }    
    
     command = "AT+SAPBR=2,1\r";
     response = GSM_getResponse(command,1000,"+SAPBR");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
        
     command = "AT+CGATT=1\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     } 
     
     command = "AT+CGACT=1,1\r";
     response = GSM_getResponse(command,5000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     } 
     
     command = "AT+CSQ\r";
     response = GSM_getResponse(command,1000,"CSQ");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     } 
     
     command = "AT+CIPSTATUS\r";
     GSM_Send_Command_UART(command, strlen(command));
     delay_ms(1000);
     response = GSM_Read_UART();
     
     command = "AT+CIICR\r";
     response = GSM_getResponse(command,5000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+CIFSR\r";
     GSM_Send_Command_UART(command, strlen(command));
     delay_ms(1000);
     response = GSM_Read_UART();
     
     command = "AT+CIPSPRT=1\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     return true;
}

bool GSM_initHTTP(void)
{
  
     char* response = " ";
     char* command = " "; 
      
     command = "AT+HTTPINIT\r";
     response = GSM_getResponse(command,500,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPSSL=1\r";
     response = GSM_getResponse(command,500,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPPARA=\"CID\",1\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPPARA=\"REDIR\",1\r";
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPPARA=\"URL\",\"https://api.tago.io/data:443\"\r";
     response = GSM_getResponse(command,2000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r";
     response = GSM_getResponse(command,2000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPPARA=\"USERDATA\",\"device-token: aa44e174-98d3-47bd-907b-f4ada1bd4711\"\r";
     response = GSM_getResponse(command,2000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPPARA=\"TIMEOUT\",1000\r";
     response = GSM_getResponse(command,2000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     return true;
}

bool GSM_sendData(void)
{
  
     char* response = " ";
     char* command = " "; 
     char *data = "{\"test_payload\": \"hello,world!\"}\r";
     
     char commandConcat[40];
     
     sprintf(commandConcat, "AT+HTTPDATA=%d,5000\r", strlen(data));
    
     GSM_Send_Command_UART(commandConcat, strlen(commandConcat));
     
     response = GSM_getResponse(data,5000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPACTION=1\r";
     response = GSM_getResponse(command,5000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }
     
     command = "AT+HTTPREAD=0,50\r";
     response = GSM_getResponseFromServer(command);
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     }  
     
     return true;
}

bool GSM_finishHTTP(void)
{
     char* response = " ";
     char* command = " "; 
     command = "AT+HTTPTERM\r";
     
     response = GSM_getResponse(command,1000,"OK");
     
     if (strcmp(response,"Timeout") == 0)
     {
       return false;
     } 
     
     return true;
}
