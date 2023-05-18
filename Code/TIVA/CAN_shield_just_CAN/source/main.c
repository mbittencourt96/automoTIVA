#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

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

#define SUPPORTED_PIDS 0
#define ENGINE_RPM 12
#define ENGINE_TEMPERATURE 5
#define THROTTLE_POS 17

//flags

volatile bool rxFlag = 0; // msg received flag
volatile bool txFlag = 0; // msg received flag
volatile bool errFlag = 0; // error flag
int receivedMsg = 0;

//system clock
uint32_t g_ui32SysClock;

//bytes of CAN message
int firstByte = 0;
int secondByte = 0;

//CAN message objs
tCANMsgObject msgRx; // the CAN msg Rx object
tCANMsgObject msgTx; // the CAN msg Tx object

//The bytes of Tx Message
unsigned int msgDataTx; // the message data is four bytes long which we can allocate as an int32

//Pointer to CAN Tx message Object
unsigned char *msgDataTxPtr = (unsigned char *)&msgDataTx; // make a pointer to msgDataTx so we can access individual bytes

//Buffer for received data
unsigned char msgDataRx[8];

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
                  txFlag = 1;
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
          txFlag = 1;  //set Tx Flag
          errFlag = 0; // clear any error flags
        }
}

//Delay function
void delay(unsigned int milliseconds) {
	SysCtlDelay((g_ui32SysClock / 3) * (milliseconds / 1000.0f));
}


//Set and configure clock and GPIO
void configureClockAndGPIO()
{
        g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                              SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 16000000);

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
        
	CANInit(CAN1_BASE);
	CANBitRateSet(CAN1_BASE,g_ui32SysClock, 500000);  //Set 500kbps CAN
        CANIntRegister(CAN1_BASE, CANIntHandler); // use dynamic vector table allocation
        IntMasterEnable();
	CANIntEnable(CAN1_BASE, CAN_INT_MASTER | CAN_INT_ERROR);
	IntEnable(INT_CAN1);
}

void requestPID(int pid)

{
          //Request PID
          
          msgDataTxPtr[0] = 2;
          msgDataTxPtr[1] = 1;
          msgDataTxPtr[2] = pid;
          msgDataTxPtr[3] = 0;
          
          msgTx.ui32MsgID = 0x7DF;  //Request ID
          //msgTx.ui32MsgIDMask = 0;
          msgTx.ui32Flags = MSG_OBJ_TX_INT_ENABLE;
          msgTx.ui32MsgLen = sizeof(msgDataTxPtr); // allow up to 8 bytes
          msgTx.pui8MsgData = msgDataTxPtr;
          
          printf("Sending request for PID  %d\n", pid);
          
          CANMessageSet(CAN1_BASE, 1, &msgTx, MSG_OBJ_TYPE_TX);
          
 
}

int main(void) {
        
        configureClockAndGPIO();
        
	// Use ID and mask 0 to recieved messages with any CAN ID
	msgRx.ui32MsgID = 0x7E7;
	//msgRx.ui32MsgIDMask = 0;
	msgRx.ui32Flags = MSG_OBJ_RX_INT_ENABLE;
	msgRx.ui32MsgLen = 8; // allow up to 8 bytes
        msgRx.pui8MsgData = msgDataRx;
        
        //Init var for Tx messages  
        msgDataTx = 0;
        CANEnable(CAN1_BASE);
        
        // Load msg into CAN peripheral message object 2 so it can trigger interrupts on any matched rx messages
        CANMessageSet(CAN1_BASE, 2, &msgRx, MSG_OBJ_TYPE_RX);
        
        int option = -1;
       
        printf("\nType the number corresponding to the info to request from ECU:\n");
        printf("0-Engine Coolant Temperature\n");
        printf("1-Engine RPM\n");
        printf("2-Throttle Position\n");
        printf("3-Supported PIDs\n");
        scanf("%d",&option);
        
        if (option == 0)
        {
          requestPID(ENGINE_TEMPERATURE); //request engine temperature
        }
        else if (option == 1)
        {
          requestPID(ENGINE_RPM); //request engine rpm
        }
        else if (option == 2)
        {
          requestPID(THROTTLE_POS); //request throttle position
        }
        else if (option == 3)
        {
          requestPID(SUPPORTED_PIDS); //request engine rpm
        }
        else
        {
          printf("Choose a valid option!\n");
        }
        while (1)
        {
          
          option = -1;
       
          if (receivedMsg == 1)
            {
                receivedMsg = 0;
                
                printf("Type the number corresponding to the info to request from ECU:\n");
                printf("0-Engine Coolant Temperature\n");
                printf("1-Engine RPM\n");
                printf("2-Throttle Position\n");
                printf("3-Supported PIDs\n");
                scanf("%d",&option);
                
                if (option == 0)
                {
                  requestPID(ENGINE_TEMPERATURE); //request engine temperature
                }
                else if (option == 1)
                {
                  requestPID(ENGINE_RPM); //request engine rpm
                }
                else if (option == 2)
                {
                  requestPID(THROTTLE_POS); //request throttle position
                }
                else if (option == 3)
                {
                  requestPID(SUPPORTED_PIDS); //request engine rpm
                }
                else
                {
                  printf("Choose a valid option!\n");     
                }
          }
          
          if (rxFlag)      
           {
          
                  msgRx.pui8MsgData = msgDataRx; // set pointer to rx buffer
                  CANMessageGet(CAN1_BASE, 2, &msgRx, 0); // read CAN message object 2 from CAN peripheral

                  rxFlag = 0; // clear rx flag

                  if(msgRx.ui32Flags & MSG_OBJ_DATA_LOST) { // check msg flags for any lost messages
                          printf("CAN message loss detected\n");
                  }

                  // read in data from rx buffer
                  firstByte = msgDataRx[3];
                  secondByte = msgDataRx[4];

                  // write to UART for debugging
                  printf("Received msg\t: %d\t %d\t\n", msgDataRx[3], msgDataRx[4]);
                  
                  receivedMsg = 1;
           }
       }
        
     return 0;
}