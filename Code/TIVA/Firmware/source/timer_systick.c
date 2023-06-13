/**************************************************************************/
/*!
    timer_systick.c (Source)

  Implementation of Timer for the AutomoTIVA project implemented in TM4C1294XL Board

  License: TBD

  By: Mariana Junghans, from UTFPR Curitiba
*/
/**************************************************************************/

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/pwm.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "timer_systick.h"   

volatile int contadorSysTick = 0;
    
void setupTimer(void)
{
  SysTickIntEnable();
  
  SysTickPeriodSet(119999);   //Sets the period of SysTick timer
}

extern void contadorTempo(void)
{
  contadorSysTick++; 
}

void delay_s(int seconds)

{
  SysTickEnable();
      
   while (contadorSysTick < 1000 * seconds)
    {
    }
     
  SysTickDisable();
  
  contadorSysTick = 0;
}


void delay_ms(int milliseconds)

{
  SysTickEnable();
      
   while (contadorSysTick < milliseconds)
    {
    }
     
  SysTickDisable();
  
  contadorSysTick = 0;
}
