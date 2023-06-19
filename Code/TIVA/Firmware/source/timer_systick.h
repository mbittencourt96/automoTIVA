#ifndef _TIMER_SYSTICK_H_
#define _TIMER_SYSTICK_H_

/**************************************************************************/
/*!
  timer_systick.h
  Timer related function definitions for use with the TM4C1294XL Board (TIVA)

  License: GNU General Public License v3.0

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

void setupTimer(void);
void delay_s(int seconds);
void delay_ms(int milliseconds);

#endif

