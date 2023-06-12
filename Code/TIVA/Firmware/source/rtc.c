/**************************************************************************/
/*!
  RTC.c (Source)

  Adaptation of the Arduino Library RTCLib by JeeLab (https://github.com/adafruit/RTClib) for the TM4C1294XL Board (TIVA)

  License: TBD

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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

#include "rtc.h"

volatile bool I2CIntFlag = false;
volatile uint32_t ui32DataRx;
uint32_t ui32SysClock;
uint8_t  ui8Count, ui8MasterBytesLength;
uint32_t ui32TxArbSize, ui32RxArbSize;
bool     bError;
  
//*****************************************************************************
//
// Enumerated Data Types for Master State Machine
//
//*****************************************************************************
enum I2C_MASTER_STATE
{
	I2C_OP_IDLE = 0,
	I2C_OP_TXADDR,
	I2C_OP_FIFO,
	I2C_OP_TXDATA,
	I2C_OP_RX_BEGIN,
        I2C_OP_RXDATA,
	I2C_OP_STOP,
	I2C_ERR_STATE
};
 
volatile uint8_t g_ui8SlaveWordAddress;
uint8_t  g_ui8MasterTxData[NUM_OF_I2CBYTES];
uint8_t  g_ui8MasterRxData[NUM_OF_I2CBYTES];
volatile uint8_t  g_ui8MasterCurrState;
volatile uint8_t  g_ui8MasterPrevState;
static volatile bool g_bI2CDirection;
volatile bool     g_bI2CRepeatedStart;
volatile uint8_t  g_ui8MasterBytes  	 = NUM_OF_I2CBYTES;
volatile uint8_t  g_ui8MasterBytesLength = NUM_OF_I2CBYTES;
volatile uint8_t  g_ui8TxBufferAvail;
volatile uint8_t  g_ui8RxBufferAvail;


  

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];

//Initialization function
  
void RTC_begin_I2C(uint32_t g_ui32SysClock)
  {     
     //
    // Enable GPIO for Configuring the I2C Interface Pins
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    
    //
    // Wait for the Peripheral to be ready for programming
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOL));
    
    //
    // Configure GPIO Pin PL4 for Interrupt Time Processing
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_4);
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_4, 0x0);

    //Enable GPIO for Configuring the I2C Interface Pins
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    //
    // Wait for the Peripheral to be ready for programming
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    //
    // Configure Pins for I2C5 Master Interface
    //
    GPIOPinConfigure(GPIO_PB4_I2C5SCL);
    GPIOPinConfigure(GPIO_PB5_I2C5SDA);
    GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_5);
    GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_4);
    
    // Stop the Clock, Reset and Enable I2C Module
    // in Master Function
    SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C5);
    SysCtlPeripheralReset(SYSCTL_PERIPH_I2C5);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C5);
    //
    // Wait for the I2C5 module to be ready.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C5))
    {
    }
    
    //
    // Initialize Master
    //
    I2CMasterInitExpClk(I2C5_BASE, g_ui32SysClock, true);
    
        
    // Assign the Transmit and Receive FIFO to the Master
    // Transmit threshold of 2 means that when there are less
    // than or equal to 2 bytes in the TX FIFO then generate
    // an interrupt
    // Receive threshold of 6 means that when there are more
    // than or qual to 6 bytes in the RX FIFO then generate
    // an interrupt
    //
    I2CTxFIFOConfigSet(I2C5_BASE, I2C_FIFO_CFG_TX_MASTER | I2C_FIFO_CFG_TX_TRIG_8);
    I2CRxFIFOConfigSet(I2C5_BASE, I2C_FIFO_CFG_RX_MASTER | I2C_FIFO_CFG_RX_TRIG_6);
    
    //
    // Transmit Buffer Space is calculated as 8 (Max depth of FIFO) - Trigger Level
    //
    g_ui8TxBufferAvail  = 8;

    //
    // Receive Buffer is calculated as the Trigger Level to read back data.
    //
    g_ui8RxBufferAvail  = 6;
        
    //
    // Flush any existing data in the FIFO
    //
    I2CTxFIFOFlush(I2C5_BASE);
    I2CRxFIFOFlush(I2C5_BASE);
    
    IntRegister(INT_I2C5,I2CIntHandler_FIFO);
    
    // Enable Interrupts for Arbitration Lost, Stop, NAK,
    // Clock Low Timeout and Data.
    //
    I2CMasterIntEnableEx(I2C5_BASE, (I2C_MASTER_INT_RX_FIFO_REQ |
			I2C_MASTER_INT_TX_FIFO_REQ | I2C_MASTER_INT_ARB_LOST |
			I2C_MASTER_INT_STOP | I2C_MASTER_INT_NACK |
			I2C_MASTER_INT_TIMEOUT | I2C_MASTER_INT_DATA));
    
    //Enable interrupt in the I2C5 peripheral
    IntEnable(INT_I2C5);
    
    I2CMasterEnable(I2C5_BASE);
    //
    //Enable interrupts to the processor
    IntMasterEnable();
    
    I2CMasterGlitchFilterConfigSet(I2C5_BASE, I2C_MASTER_GLITCH_FILTER_8);
  
    //
    // Initialize and Configure the Master Module State Machine
    //
    g_ui8MasterCurrState = I2C_OP_IDLE;

    //
    // Check if the Bus is Busy or not
    //
    while(I2CMasterBusBusy(I2C5_BASE));
 };

//I2C Interrupt Handler with FIFO

extern void I2CIntHandler_FIFO(void)
{
    uint32_t ui32I2CMasterInterruptStatus;
    uint8_t  ui8Loop;

    //
    // Toggle PL4 High to Indicate Entry to ISR
    //
    GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_4, GPIO_PIN_4);

    //
    // Get the masked interrupt status and clear the flags
    //
    ui32I2CMasterInterruptStatus = I2CMasterIntStatusEx(I2C5_BASE, true);

      //
      // Execute the State Machine
      //
      switch (g_ui8MasterCurrState) {
      case I2C_OP_IDLE:
              //
              // Move from IDLE to Transmit Address State
              //
              g_ui8MasterPrevState = g_ui8MasterCurrState;
              g_ui8MasterCurrState = I2C_OP_FIFO;

              //
              // Write the upper bits of the page to the Slave
              //
              I2CMasterSlaveAddrSet(I2C5_BASE, RTC_ADDRESS, false);
              I2CMasterDataPut(I2C5_BASE, g_ui8SlaveWordAddress);
              I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_START);
              break;
              
      case I2C_OP_FIFO:
              //
              // If Last Data has been NAK'ed then go to stop state
              //
              if(ui32I2CMasterInterruptStatus & I2C_MASTER_INT_NACK)
              {
                      g_ui8MasterCurrState = I2C_OP_STOP;
              }
              //
              // Based on the direction move to the appropriate state
              // of Transmit or Receive. Also send the BURST command
              // for FIFO Operations.
              //
              else if(!g_bI2CDirection)
              {
                      g_ui8MasterCurrState = I2C_OP_STOP;
                      I2CMasterDataPut(I2C5_BASE,g_ui8MasterTxData[0]);
                      I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
              }
              else
              {
                      g_ui8MasterCurrState = I2C_OP_RXDATA;
                      I2CMasterSlaveAddrSet(I2C5_BASE, RTC_ADDRESS, true);
                      I2CMasterControl(I2C5_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
              }
              break;

      case I2C_OP_RXDATA:
              //
              // Move the current state to the previous state
              // Else continue with the transmission till last byte
              //
              g_ui8MasterPrevState = g_ui8MasterCurrState;

              //
              // If Address has been NAK'ed then go to stop state
              // If a Stop condition is seen due to number of bytes getting
              // done then move to STOP state and read the last data byte
              //
              if(ui32I2CMasterInterruptStatus & I2C_MASTER_INT_NACK)
              {
                      g_ui8MasterCurrState = I2C_OP_STOP;
              }
              else if(ui32I2CMasterInterruptStatus & I2C_MASTER_INT_STOP)
              {
                      g_ui8MasterCurrState = I2C_OP_STOP;
                      g_ui8MasterRxData[0] = I2CMasterDataGet(I2C5_BASE);
               }
              else if(ui32I2CMasterInterruptStatus & I2C_MASTER_INT_RX_FIFO_REQ)
              {        
                g_ui8MasterCurrState = I2C_OP_STOP;
                g_ui8MasterRxData[0] = I2CFIFODataGet(I2C5_BASE);
              }
              else if (ui32I2CMasterInterruptStatus & I2C_MASTER_INT_DATA)
              {
                g_ui8MasterCurrState = I2C_OP_STOP;
                g_ui8MasterRxData[0] = I2CMasterDataGet(I2C5_BASE);
              }
               else if (ui32I2CMasterInterruptStatus & (I2C_MASTER_INT_DATA | I2C_MASTER_INT_STOP))
              {
                g_ui8MasterCurrState = I2C_OP_STOP;
                g_ui8MasterRxData[0] = I2CMasterDataGet(I2C5_BASE);
              }
              else
              {
                      g_ui8MasterCurrState = I2C_ERR_STATE;
              }
              break;

      case I2C_OP_STOP:
              //
              // Move the current state to the previous state
              // Else continue with the transmission till last byte
              //
              g_ui8MasterPrevState = g_ui8MasterCurrState;
              break;

      case I2C_ERR_STATE:
              g_ui8MasterCurrState = I2C_ERR_STATE;
              break;

      default:
              g_ui8MasterCurrState = I2C_ERR_STATE;
              break;
      }

      //
      // Toggle PL4 Low to Indicate Exit from ISR
      //
      I2CMasterIntClearEx(I2C5_BASE, ui32I2CMasterInterruptStatus);
      GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_4, 0x0);
}

void RTC_write_I2C(uint8_t b,uint8_t reg)
{
        g_ui8MasterTxData[0]   = b;
        g_ui8MasterRxData[0]   = 0xFF;

  //
  // Set the I2CMBLEN register and also initialize
  // the internal flag
  //
  ui8MasterBytesLength = 1;

  I2CMasterBurstLengthSet(I2C5_BASE, ui8MasterBytesLength);

  //
  // Set Transmit Flag and set the Page Address in
  // external slave to 0x0000
  //
  g_bI2CDirection = false;
  g_ui8SlaveWordAddress = reg;
  g_ui8MasterBytes       = 0;

  //
  // Trigger the Transfer using Software Interrupt
  //
  g_ui8MasterCurrState = I2C_OP_IDLE;
  IntTrigger(INT_I2C5);
  while(g_ui8MasterCurrState != I2C_OP_STOP);
} 

void RTC_read_I2C(uint8_t reg)
{

  //
  // Set the I2CMBLEN register and also initialize
  // the internal flag
  //
  ui8MasterBytesLength = 1;

  I2CMasterBurstLengthSet(I2C5_BASE, ui8MasterBytesLength);
  //
  // Set receive Flag and set the Page Address in
  // external slave to the proper register
  //
  g_bI2CDirection = true;
  g_bI2CRepeatedStart    = true;
  g_ui8SlaveWordAddress = reg;
  g_ui8MasterBytes       = 0;

  //
  // Trigger the Transfer using Software Interrupt
  //
  g_ui8MasterCurrState = I2C_OP_IDLE;
  IntTrigger(INT_I2C5);
    while(g_ui8MasterCurrState != I2C_OP_STOP);   
}

//Convert binary value to BCD

uint8_t bin2bcd(uint8_t val)
{ 
  uint8_t bcd_value = val + 6 * (val / 10); 
  return bcd_value; 
}

//Convert BCD value to binary

uint8_t bcd2bin(uint8_t val) { 
  return val - 6 * (val >> 4); 
}

/*Get Day of week */
uint8_t dowToDS3231(uint8_t d) { 
  return d == 0 ? 7 : d; 
}

/* Adjust time to RTC module */
void RTC_adjust_time(uint16_t year, uint8_t month, uint8_t dow, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
  RTC_write_I2C(bin2bcd(second),0x00);
  RTC_write_I2C(bin2bcd(minute),0x01);
  
  
  RTC_write_I2C(bin2bcd(hour),0x02);
  
  RTC_read_I2C(0x02);
 //Get value from registers
  int hours = (int) bcd2bin(g_ui8MasterRxData[0]);
  
  RTC_write_I2C(bin2bcd(dow),0x03); 
  RTC_write_I2C(bin2bcd(day),0x04); 
  RTC_write_I2C(bin2bcd(month),0x05); 
  RTC_write_I2C(bin2bcd(year),0x06); 
  
  RTC_read_I2C(RTC_STATUSREG);
  uint8_t status_reg = g_ui8MasterRxData[0];
  status_reg &= ~0x80; // flip OSF bit
  RTC_write_I2C(status_reg,RTC_STATUSREG);  
};

char* RTC_now(void)
{
  RTC_read_I2C(0x00);
 //Get value from registers
  int seconds = (int) bcd2bin(g_ui8MasterRxData[0]);
  
  if (seconds > 59 || seconds < 0)
  {
    return "Error";
  }
 
  RTC_read_I2C(0x01);
  int minutes = (int) bcd2bin(g_ui8MasterRxData[0]);
  
  if (minutes > 59 || minutes < 0)
  {
    return "Error";
  }
  RTC_read_I2C(0x02);
  int hours = (int) bcd2bin(g_ui8MasterRxData[0]);
  if (hours > 24 || hours < 0)
  {
    return "Error";
  }
  RTC_read_I2C(0x04);
  int day = (int) bcd2bin(g_ui8MasterRxData[0]);
  
  if (day > 31 || day < 1)
  {
    return "Error";
  }
  RTC_read_I2C(0x05);
  int month = (int) bcd2bin(g_ui8MasterRxData[0]);
  
  if (month > 12 || month < 1)
  {
    return "Error";
  }
  
  int year = 2023;
  
  int length = snprintf( NULL, 0, "%d", seconds );
  char* seconds_str = malloc( length + 1 );
  snprintf( seconds_str, length + 1, "%d", seconds );

  length = snprintf( NULL, 0, "%d", minutes );
  char* minutes_str = malloc( length + 1 );
  snprintf( minutes_str, length + 1, "%d", minutes );
  
  length = snprintf( NULL, 0, "%d", hours );
  char* hours_str = malloc( length + 1 );
  snprintf( hours_str, length + 1, "%d", hours );
  
  length = snprintf( NULL, 0, "%d", day );
  char* day_str = malloc( length + 1 );
  snprintf( day_str, length + 1, "%d", day );
  
  length = snprintf( NULL, 0, "%d", month );
  char* month_str = malloc( length + 1 );
  snprintf( month_str, length + 1, "%d", month );
  
  char* year_str = "2023";
  
  char datetime [20];
  
  char bar = '/';
  char dots = ':';
  char space = ' ';
  char line = '-';
  char dot = '.';
 
  //Build the string
  strncat(datetime, year_str, 4);
  strncat(datetime, &line, 1);
  if (strlen(month_str) == 1)
  {
    strncat(datetime, "0", 1);
  }
  strncat(datetime, month_str, 2);
  strncat(datetime, &line, 1);
  if (strlen(day_str) == 1)
  {
    strncat(datetime, "0", 1);
  }
  strncat(datetime, day_str, 2);
  strncat(datetime, "T", 1);
  if (strlen(hours_str) == 1)
  {
    strncat(datetime, "0", 1);
  }
  strncat(datetime, hours_str, 2);
  strncat(datetime, &dots, 1);
  if (strlen(minutes_str) == 1)
  {
    strncat(datetime, "0", 1);
  }
  strncat(datetime, minutes_str, 2);
  strncat(datetime, &dots, 1);
   if (strlen(seconds_str) == 1)
  {
    strncat(datetime, "0", 1);
  }
  strncat(datetime, seconds_str, 2);
  
  return datetime;
}

