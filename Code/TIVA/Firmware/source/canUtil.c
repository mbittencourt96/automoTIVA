//CAN related function declarations

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

#define ENGINE_RPM 12
#define ENGINE_TEMPERATURE 5
#define THROTTLE_POS 17
#define ETH_PERCENTAGE 82
#define FUEL_LEVEL 47
#define VEHICLE_SPEED 13
#define ODOMETER 49



//Flag of received data
volatile bool rxFlag = 0;
//Error flag
volatile bool errFlag = 0;

//bytes of CAN message
int firstByte = 0;
int secondByte = 0;

//CAN message objs
tCANMsgObject msgRx; // the CAN msg Rx object
tCANMsgObject msgTx; // the CAN msg Tx object

//The bytes of Tx Message
unsigned int msgDataTx; // the message data is four bytes long which we can allocate as an int32

//Pointer to CAN Tx message Object
uint8_t *msgDataTxPtr; // make a pointer to msgDataTx so we can access individual bytes

//Buffer for received data
unsigned char msgDataRx[8];


void requestPID(int pid)

{
          //Request PID    
          msgDataTxPtr[0] = 2;
          msgDataTxPtr[1] = 1;
          msgDataTxPtr[2] = pid;
          msgDataTxPtr[3] = 0;
          
          //printf("Sending request for PID  %d\n", pid);
          
          CANMessageSet(CAN1_BASE, 1, &msgTx, MSG_OBJ_TYPE_TX);
}

float convertOBDData(char firstByte, char secondByte, int pid)
{
  if (pid == ENGINE_TEMPERATURE)
  {
    float ect = firstByte - 40;
    return ect;
  }
  else if (pid == ENGINE_RPM)
  {
    float rpm = ((256*firstByte)+secondByte)/4;
    return rpm;
  }
  else if (pid == THROTTLE_POS)
  {
    float tp = (100/255)*firstByte;
    return tp;
  }
  else if (pid == ETH_PERCENTAGE)
  {
    float ep = (100/255)*firstByte;
    return ep;
  }
  else if (pid == FUEL_LEVEL)
  {
    float fl = (100/255)*firstByte;
    return fl;
  }
  else if (pid == VEHICLE_SPEED)
  {
    return firstByte;
  }
  else
  {
    float dc = 256*firstByte + secondByte;
    return dc;
  }
}

float readCANmessage()
{
  if (rxFlag == 1)
  {
    
    rxFlag = 0;  
    msgRx.pui8MsgData = msgDataRx; // set pointer to rx buffer
    CANMessageGet(CAN1_BASE, 2, &msgRx, 0); // read CAN message object 2 from CAN peripheral
    
    if(msgRx.ui32Flags & MSG_OBJ_DATA_LOST) { // check msg flags for any lost messages
              return -1;
    }

    // read in data from rx buffer
    firstByte = msgDataRx[3];
    secondByte = msgDataRx[4];
      
    float value = convertOBDData(msgDataRx[3],msgDataRx[4],msgDataRx[2]);
    return (int)value;

  }
  else 
  {
    return -1;
  }
}

void initCANMessages(void)
{
    //Define ECU Address
    msgRx.ui32MsgID = 0;
    msgRx.ui32MsgIDMask = 0;
    msgRx.ui32Flags = MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER;
    msgRx.ui32MsgLen = 8; // allow up to 8 bytes
    msgRx.pui8MsgData = msgDataRx;
    
    //Init var for Tx messages  
    msgDataTx = 0;  
    // Load msg into CAN peripheral message object 2 so it can trigger interrupts on any matched rx messages
    CANMessageSet(CAN1_BASE, 2, &msgRx, MSG_OBJ_TYPE_RX);
    
    msgDataTxPtr = (uint8_t*) &msgDataTx;
      
    msgTx.ui32MsgID = 0x7DF;  //Request ID
    msgTx.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
    msgTx.ui32MsgIDMask = 0;
    msgTx.ui32MsgLen = sizeof(msgDataTxPtr); // allow up to 8 bytes
    msgTx.pui8MsgData = msgDataTxPtr;
}

// CAN interrupt handler
void CANIntHandler(void) {

	unsigned long status = CANIntStatus(CAN1_BASE, CAN_INT_STS_CAUSE); // read interrupt status

	if(status == CAN_INT_INTID_STATUS) { // controller status interrupt
		status = CANStatusGet(CAN1_BASE, CAN_STS_CONTROL);
                if (status == CAN_STATUS_RXOK)
                {
                  unsigned long status_msg = CANIntStatus(CAN1_BASE, CAN_INT_STS_OBJECT); // read interrupt status of message object
                  if (status_msg == 2)
                  {
                    CANIntClear(CAN1_BASE, 2); // clear interrupt
                    rxFlag = 1; // set rx flag
                    errFlag = 0; // clear any error flags
                  }
                }
                else if (status == CAN_STATUS_TXOK)
                {
                  CANIntClear(CAN1_BASE, 1); // clear interrupt
                  errFlag = 0; // clear any error flags
                }
                else
                {
                    errFlag = 1;
                    CANIntClear(CAN1_BASE, status);
                }
         }
        else if (status == 2)  //Caused by Rx
        {
          CANIntClear(CAN1_BASE, 2); // clear interrupt
          rxFlag = 1; // set Rx flag
          errFlag = 0; // clear any error flags
        }
        else if (status == 1)  //Caused by Tx
        {
          CANIntClear(CAN1_BASE, 1); // clear interrupt
          errFlag = 0; // clear any error flags
        }
}

void configureCAN(uint32_t g_ui32SysClock)
{
	// Set up CAN1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
        
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB))
          {
          }
        
	GPIOPinConfigure(GPIO_PB0_CAN1RX);
	GPIOPinConfigure(GPIO_PB1_CAN1TX);
	GPIOPinTypeCAN(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_CAN1);
        
        while(!SysCtlPeripheralReady(SYSCTL_PERIPH_CAN1))
          {
          }
        
        CANDisable(CAN1_BASE);
	CANInit(CAN1_BASE);
	CANBitRateSet(CAN1_BASE,g_ui32SysClock, 500000);  //Set 500kbps CAN
        CANIntRegister(CAN1_BASE, CANIntHandler); // use dynamic vector table allocation
        IntMasterEnable();
	CANIntEnable(CAN1_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
	IntEnable(INT_CAN1);
        
        //Enable CAN peripheral
        CANEnable(CAN1_BASE);
}