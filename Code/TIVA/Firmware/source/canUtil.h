
/**************************************************************************/
/*!
  canUtil.h
  CAN related function definitions for the TM4C1294XL Board (TIVA)

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/


#ifndef _CANUTIL_H_
#define _CANUTIL_H_


void CANIntHandler(void);
void initCANMessages(void);
void requestPID(int pid);
float convertOBDData(char firstByte, char secondByte, int pid);
void configureCAN(uint32_t g_ui32SysClock);
float readCANmessage(void);


#endif