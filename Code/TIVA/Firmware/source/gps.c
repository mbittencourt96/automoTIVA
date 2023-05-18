/**************************************************************************/
/*!
  GPS.c (Source)

  Implementation of GPS-related functions (using NEO 6M module) for the TM4C1294XL Board (TIVA)

  License: TBD

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "gps.h"

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


void GPS_setup_UART(uint32_t g_ui32SysClock,int baud)
{
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART2) && !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA))
  {
  }                                                 //Wait until UART6 and PORT P is ready
  
  GPIOPinConfigure(GPIO_PA6_U2RX);
  GPIOPinConfigure(GPIO_PA7_U2TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  UARTConfigSetExpClk(GPS_UART, g_ui32SysClock, baud, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
  
  UARTEnable(GPS_UART);
}

char* GPS_Read_UART(void)
{
  char caract;
  char* receivedString = (char*) malloc(100*sizeof(char));
  int i = 0;
  
  if (UARTCharsAvail(GPS_UART))
  {
   while(caract != '\n')
   {
      caract = (unsigned char) UARTCharGetNonBlocking(GPS_UART);
      
      if (caract != '\n')
      {
        receivedString[i++] = caract;
      }      
    }
   
  
    receivedString[i] = '\0';
   
   return receivedString;
  }
  else
  {
    return "GPS not available";
  }
}

char* GPS_parse_coordinate(char* c)
{
  //Parse latitude or longitude
  
  char degrees[3];
  char minutes[2];
  char seconds[4];
  char decimals[4];
  static char outputLatLong[10];
  float dec_minutes;
  float dec_seconds;
  float dec_total;
  char minus = '-';
  
  if (c[0] == '-')
  {
    strncat(outputLatLong,&minus,1);
  }
  
  if (c[1] == '0')
  {
    degrees[0] = c[2];
    degrees[1] = c[3];
  }
  else
  {
    degrees[0] = c[1];
    degrees[1] = c[2];
    degrees[2] = c[3];
  }
  
  strncat(outputLatLong,degrees,strlen(degrees));
  
  minutes[0] = c[3];
  minutes[1] = c[4];
  
  dec_minutes = atoi(minutes)/60;   //Convert minutes to decimals
  
  seconds[0] = c[6];
  seconds[1] = c[7];
  seconds[2] = c[8];
  seconds[3] = c[9];
  
  dec_seconds = atoi(seconds);
  dec_seconds = (dec_seconds / 10000) * 60;  //Convert decimals to seconds
  dec_seconds = dec_seconds / 3600; //Convert seconds to decimals 
  
  dec_total = dec_seconds + dec_minutes; //Sum the decimals 
  
  int dec_total_int = (int) dec_total * 10000;  //Multiply by 10000 and convert to integer to allow conversion to string
 
  char decimals_str[4];
  sprintf(decimals_str, "%d", dec_total_int);  //Convert decimals from integer to string to allow concatenation
   
  char dot = '.';
  
  strncat(outputLatLong,&dot,1);
  strncat(outputLatLong,decimals_str,strlen(decimals_str));
  
  outputLatLong[9] = '\0';
  
  return outputLatLong;
}
 
char* GPS_get_info(char *rawString)
{
  char header[7];
  char time[6];
  char date[6];
  char latitude[11];
  char longitude[12];
  char outputString [150];
  
  char caract = ' ';
  
  int comp1;
  int comp2;
  int comp3;
  int comp4;
  
  int i = 0;
  
  while(caract != ',')
  {
    caract = rawString[i];
    if (caract != ',')
    {
      header[i] = caract;
    }
    i++;
  }
 

  header[6] = '\0';
  
  caract = ' ';
  
  comp1 = strcmp(header,"$GPGGA");
  comp2 = strcmp(header,"$GNGGA"); 
  comp3 = strcmp(header,"$GPRMC");
  comp4 = strcmp(header,"$GNRMC"); 
  
  if (comp1 == 0 || comp2 == 0)
  {
    int j = 0;
    
    while(caract != '.')
      {
        caract = rawString[i];
        time[j] = caract;
        i++;
        j++;
      }
    
    
    j = 0;
    caract = ' ';
    
    while(rawString[i] != ',')
    {
      i++;
    }
    
    i++;
    
    while(caract != ',')
      {
        caract = rawString[i];
        latitude[j+1] = caract;
        i++;
        j++;
      }
    
    i++;
     
    if (rawString[i] == 'S')    //South
    {
      latitude[0] = '-';
    }
    else      //North
    {
      latitude[0] = '+';
    }
    
    i = i + 2;
    j = 0;
    
    while(caract != ',')
    {
      caract = rawString[i];
      longitude[j+1] = caract;
      i++;
      j++;
    }
    
    j = 0;
    
    i++;
    
    if (rawString[i] == 'W')    //West
    {
      longitude[0] = '-';
    }
    else      //East
    {
      longitude[0] = '+';
    }
  }
 
 else if ( comp3 == 0 || comp4 == 0)
  {
    int j = 0;
    
    while(caract != '.')
      {
        caract = rawString[i];
        time[j] = caract;
        i++;
        j++;
      }
    
    time[6] = '\0';
    
    j = 0;
    caract = ' ';
     
    while(rawString[i] != ',')
    {
      i++;
    }
    
    i = i + 3;
    
    while(caract != ',')
      {
        caract = rawString[i];
        latitude[j+1] = caract;
        i++;
        j++;
      }
     
    if (rawString[i] == 'S')    //South
    {
      latitude[0] = '-';
    }
    else      //North
    {
      latitude[0] = '+';
    }
    
    i = i + 2;
    j = 0;
    caract = ' ';
    
    while(caract != ',')
    {
      caract = rawString[i];
      longitude[j+1] = caract;
      i++;
      j++;
    }
    
    if (rawString[i] == 'W')    //West
    {
      longitude[0] = '-';
    }
    else      //East
    {
      longitude[0] = '+';
    }
    
    i++;
    i++;
    
    while(rawString[i] != ',')
    {
      i++;
    }
    
    i++;
     
    while(rawString[i] != ',')
    {
      i++;
    }
    
    i++;
    j = 0;
     while(caract != ',')
    {
      caract = rawString[i];
      date[j] = caract;
      i++;
      j++;
    } 

  }
  else
  {
    return " ";
  }
  
  char* lat =  GPS_parse_coordinate(latitude);
  char* lng =  GPS_parse_coordinate(longitude);
   
  char slash = '/';
  char dots = ':';
  char dot = '.';
  char space = ' ';
  //Build the string
  
  strncat(outputString, "Latitude: ", strlen("Latitude: "));
 
  strncat(outputString, &latitude[0],1);
  strncat(outputString, &latitude[1],1);
  strncat(outputString, &latitude[2],1);
  strncat(outputString, &dot, 1);
  strncat(outputString, &latitude[3], 1);
  strncat(outputString, &latitude[4], 1);
  strncat(outputString, " Longitude: ", strlen(" Longitude: "));
  
  strncat(outputString, &longitude[0],1);
  strncat(outputString, &longitude[2],1);
  strncat(outputString, &longitude[3],1);
  strncat(outputString,&dot,1);
  strncat(outputString, &longitude[4],1);
  strncat(outputString, &longitude[5],1);
 
  char* strOut = (char*) outputString;
  return strOut;    
}

