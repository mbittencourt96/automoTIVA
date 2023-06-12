#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

#include "canUtil.h"
#include "uart.h"
#include "rtc.h"
#include "gps.h"
#include "rgb.h"
#include "gsm.h"
#include "timer_systick.h"
#include "dataStore.h"

//PID codes
#define ENGINE_RPM 12
#define ENGINE_TEMPERATURE 5
#define VEHICLE_SPEED 13
#define THROTTLE_POS 17
#define FUEL_LEVEL 47
#define ODOMETER 49
#define ETH_PERCENTAGE 82

int contador_erro_internet = 0;
int contador_erro_gps = 0;
int contador_erro_rtc = 0;
int contador_erro_mem = 0;

//Enumeration with possible states
typedef enum {
  CONFIG = 0,
  WAITING_PID,
  WAITING_DATE,
  WAITING_GPS,
  STORING,
  SENDING,
  SUCCESS  
} STATE;

//OBD parameters to be requested
int engine_rpm = 0;
int engine_temp = 0;
int vehicle_speed = 0;
int fuel_level = 0;
int th_pos = 0;
int odometer = 0;
int eth_percentage = 0;
int pids[7];

//System clock
uint32_t g_ui32SysClock;

STATE currentState = CONFIG;

//Final String
char pids_str[50];

 //GPS Strings
char GPS_OutputStr[100];
char location_str [50];
char* outputStr;

//Date string
char* datetime_str;
              

int counter = 0;

          
void main(void) {  
  
  while(1)
      {
          switch (currentState)
          {
            case CONFIG:  
           
              //Define system clock as 16MHz
              g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 16000000);
              
              //Configure UART
              setupUART7(g_ui32SysClock,9600);
            
              //Configure RGB Led
              setupPWM_LEDS(g_ui32SysClock);
              
              //Configure timer
              setupTimer();
              //Define as BLue LED
              Color c = BLUE;
              
               //Blink Blue LED for 2 times
              blinkLED(c,1,2);
               
              //Configure GPS Module Peripheral with 9600 bps baud rate (UART comm)
              GPS_setup_UART(g_ui32SysClock,9600);   
         
              //Configure RTC Module Peripheral (I2C Comm)
              RTC_begin_I2C(g_ui32SysClock);
              RTC_adjust_time(23,5,5,26,15,0,0);                              //Change here according to current time
              
              //Configure CAN Peripheral (CAN Controller)
              configureCAN(g_ui32SysClock);
              //Initialize CAN Bus to wait for messages
              initCANMessages();
              
              //Init Flash Memory Queue
              Queue *Segments = (Queue*) malloc(sizeof(Queue));              
              initQueue(Segments);
              eraseFlash();
             
              c = GREEN;
               
              //Blink Green LED
              blinkLED(c,1,1);
             
              currentState = WAITING_PID;

              break;
          case WAITING_PID:
              //Define as BLue LED
               c = BLUE;
      
              //Blink Blue LED for 2 times to indicate we are entering this state
              blinkLED(c,1,2);
              
              //Reset internet error counter
              contador_erro_internet = 0;
              
              //Reset RTC error counter
              contador_erro_rtc = 0;

              //Request Engine RPM PID
              requestPID(ENGINE_RPM);
              delay_s(1);
              engine_rpm = readCANmessage();  //Read message that was received
              
              //Request Engine Temperature PID
              requestPID(ENGINE_TEMPERATURE);
              delay_s(1);
              engine_temp = readCANmessage();  //Read message that was received
              
              //Request Vehicle Speed PID
              requestPID(VEHICLE_SPEED);
              delay_s(1);
              vehicle_speed = readCANmessage();  //Read message that was received
              
              //Request Throttle position PID
              requestPID(THROTTLE_POS);
              delay_s(1);
              th_pos = readCANmessage();  //Read message that was received

              //Request Fuel Level PID
              requestPID(FUEL_LEVEL);
              delay_s(1);
              fuel_level = readCANmessage();  //Read message that was received
              
              //Request Fuel Level PID
              requestPID(ODOMETER);
              delay_s(1);
              odometer = readCANmessage();  //Read message that was received

              //Request Ethanol Percentage PID
              requestPID(ETH_PERCENTAGE);
              delay_s(1);
              eth_percentage = readCANmessage();  //Read message that was received
              
              pids[0] = engine_rpm;
              pids[1] = engine_temp;
              pids[2] = vehicle_speed;
              pids[3] = th_pos;
              pids[4] = fuel_level;
              pids[5] = odometer;
              pids[6] = eth_percentage;
              
              if (engine_rpm == -1 && engine_temp == -1 &&  vehicle_speed == -1 &&
                  th_pos == -1 && fuel_level == -1 && odometer == -1)  //None of the PIDs was received
              {
                c = RED;
                blinkLED(c,1,1);
                currentState = WAITING_PID;
              }
              else   //At least one PID was received
              {                
                c = GREEN;
                //Blink Green LED
                blinkLED(c,1,1);
                currentState = WAITING_DATE;
              }      
              break;
          case WAITING_DATE:
             //Define as BLue LED
             c = BLUE;
              
             //Blink Blue LED for 2 times
             blinkLED(c,1,2);    
             //Datetime string
             datetime_str = (char*) malloc(50*sizeof(char));
             datetime_str = RTC_now();
              
              if (strcmp(datetime_str,"Error") == 0)
              {
                currentState = WAITING_DATE;
                c = RED;
                blinkLED(c,1,1);
                contador_erro_rtc++;
                
                if (contador_erro_rtc > 5)
                {
                  currentState = WAITING_PID;
                }
              }
              else
              {
                c = GREEN;
                blinkLED(c,1,1);   //Blink Green LED
                currentState = WAITING_GPS;
              }
       
              break;
          case WAITING_GPS:
           
            do{
              outputStr = GPS_Read_UART();
              contador_erro_gps++;
              if (contador_erro_gps >= 5)
              {
                break;
              }
            }while ((strstr(outputStr,"$GPGGA") == NULL && strstr(outputStr,"$GNGGA") == NULL) || strlen(outputStr) < 58);
            
            strncpy(GPS_OutputStr,outputStr,strlen(outputStr));
            
            if (contador_erro_gps < 5)
            {
              c = GREEN;
              blinkLED(c,1,1);   //Blink Green LED
              outputStr = GPS_get_info(GPS_OutputStr);           
              strncpy(location_str,outputStr,strlen(outputStr));
            }
            else
            {
              c = RED;
              blinkLED(c,1,1);   //Blink Red LED
              strncpy(location_str," ", 1);
            }
            
            int length = snprintf( NULL, 0, "%d", engine_rpm );
            char* rpm_str = (char*) malloc(length+1);
            sprintf(rpm_str, "%d", engine_rpm);

            length = snprintf( NULL, 0, "%d", vehicle_speed );
            char* veh_str = (char*) malloc(length+1);
            sprintf(veh_str, "%d", vehicle_speed);

            length = snprintf( NULL, 0, "%d", fuel_level );
            char* fuel_str = (char*) malloc(length+1);
            sprintf(fuel_str, "%d", fuel_level);
            
            length = snprintf( NULL, 0, "%d", th_pos );
            char* th_str = (char*) malloc(length+1);
            sprintf(th_str, "%d", th_pos);
            
            length = snprintf( NULL, 0, "%d", odometer );
            char* odometer_str = (char*) malloc(length+1);
            sprintf(odometer_str, "%d", odometer);
            
            length = snprintf( NULL, 0, "%d", eth_percentage);
            char* eth_str = (char*) malloc(length+1);
            sprintf(eth_str, "%d", eth_percentage);
            
            length = snprintf( NULL, 0, "%d", engine_temp);
            char* temp_str = (char*) malloc(length+1);
            sprintf(temp_str, "%d", engine_temp);
            
            strncat(pids_str, rpm_str, strlen(rpm_str));
            strncat(pids_str, "*", 1);
            strncat(pids_str, veh_str, strlen(veh_str));
            strncat(pids_str, "*", 1);
            strncat(pids_str, fuel_str, strlen(fuel_str));
            strncat(pids_str, "*", 1);
            strncat(pids_str, odometer_str, strlen(odometer_str));
            strncat(pids_str, "*", 1);
            strncat(pids_str, eth_str, strlen(eth_str));
            strncat(pids_str, "*", 1);
            strncat(pids_str, temp_str, strlen(temp_str));
            strncat(pids_str, "*", 1);
            strncat(pids_str, GPS_OutputStr, strlen(GPS_OutputStr));
            strncat(pids_str, "*", 1);
            strncat(pids_str, datetime_str, strlen(datetime_str));
            
            currentState = STORING;
            break;
          case STORING:
              contador_erro_gps = 0;
              //Blink yellow LED
              c = YELLOW;
              blinkLED(c,1,2);
              
              bool stored = storeStringInFlash(pids_str,Segments);
              
              if(stored)
              {
                c = GREEN;
                blinkLED(c,1,1);
                currentState = SENDING;
              }
              else
              {
                contador_erro_mem++;
                currentState = STORING;
              }
              
              if (contador_erro_mem > 3) 
              {
                currentState = SENDING;
                c = RED;
                blinkLED(c,1,1);
              }
             
              break;
          case SENDING:
              contador_erro_mem = 0;
              c = YELLOW;
              
              blinkLED(c,1,2);
              
              int k = 0;
              
              while(k < Segments->size)
              {
                uint32_t current_address;
                
                FlashSegment_t *curr = Segments->front;
                current_address = curr->address;
                
                char* content = FlashRead(current_address);
                
                UARTSend(UART7_BASE,content,strlen(content));   //Send string with information to the ESP8266
                
                char confirmation[5];
                
                delay_s(2);
                
                char* result;
                
                do{
                    result = UARTRead(UART7_BASE,confirmation);
                    contador_erro_internet++;
                    if (contador_erro_internet > 9)
                    {
                      break;
                    }
                  }while (strcmp(result," ") == 0);
                  
                  if (contador_erro_internet > 9)
                  {
                    break;
                  }
                  
                while(curr != NULL) 
                {
                  curr = curr->next;
                }
                
                k++;
              }
              
              if (contador_erro_internet > 9)
              {
                 currentState = WAITING_PID;
                 c = RED;
                 blinkLED(c,1,1);
              }
              else
              {
                  currentState = SUCCESS;
              }
       
              break;
          case SUCCESS:
            c = GREEN;
            blinkLED(c,1,2);
            eraseFlash();
            delay_s(10);   
            currentState = WAITING_PID;
            break;
          default: 
              break; 
          }  
          
      }
}
  
