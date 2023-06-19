#ifndef _RGB_H_
#define _RGB_H_

/**************************************************************************/
/*!
    RGB.h

  RGB LED related function definitions for the AutomoTIVA project implemented in TM4C1294XL Board

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

//Enumeration with possible colors
typedef enum {
  RED = 0,
  YELLOW,
  BLUE,
  GREEN
} Color;

void setupPWM_LEDS(uint32_t g_ui32SysClock);
void LEDturnON(Color c);
void LEDturnOFF(void);
void blinkLED(Color c, int seconds, int turns);

#endif