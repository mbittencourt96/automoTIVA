/**************************************************************************/
/*!
    RGB.c (Source)

  Implementation of RGB LED for the AutomoTIVA project implemented in TM4C1294XL Board

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
#include "rgb.h"

void setupPWM_LEDS(uint32_t g_ui32SysClock)
{    
  float PWM_WORD;
    
  PWM_WORD = (1/500)*g_ui32SysClock;   //PWM frequency = 500Hz
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);  //Enable GPIO for LEDs Red and Green
  //
  // Check if the peripheral access is enabled.
  //
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOG))
  {
  }
  
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);  //Enable GPIO for LED Blue
  //
  // Check if the peripheral access is enabled.
  //
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOK))
  {
  }
  
   GPIOPinConfigure(GPIO_PG0_M0PWM4);
   GPIOPinConfigure(GPIO_PG1_M0PWM5);
   GPIOPinConfigure(GPIO_PK5_M0PWM7);
   
   GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);   //Configure PWM outputs (PG0 and PG1) 
   
   GPIOPinTypePWM(GPIO_PORTK_BASE, GPIO_PIN_5);                //Configure PWM output (PK5)
   
   GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_0, 0x0); 
   GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_1, 0x0);
   GPIOPinWrite(GPIO_PORTK_BASE, GPIO_PIN_5, 0x0);  //Leds OFF
  
  
   SysCtlPWMClockSet(SYSCTL_PWMDIV_1); // Enable clock to PWM module
   
   SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0); // Use PWM module 0

   PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
   PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_WORD); // Set PWM period
   PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0);   // Set Initial Duty cycle 
   PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 0);   // Set Initial Duty cycle 
   
   PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
   PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, PWM_WORD); // Set PWM period
   PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, 0);   // Set Initial Duty cycle 
   
   PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT | PWM_OUT_5_BIT | PWM_OUT_7_BIT, true); // Enable PWM output channels
        
   // Enable PWM generators
   PWMGenEnable(PWM0_BASE, PWM_GEN_2);
   PWMGenEnable(PWM0_BASE, PWM_GEN_3);
}

void LEDturnON(Color c)
{
  
  switch(c)
  {
  case 0:   //Red
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 1); // 100% duty cycle for PWM4 (red)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 0); // 0% duty cycle for PWM5 (green)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_3) * 0); // 0% duty cycle for PWM7 (blue)
    
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true); // Enable PWM output channels
   
    break;
  case 1:  //Yellow
    
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 0.5); // 50% duty cycle for PWM4 (red)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 0.5); // 50% duty cycle for PWM5 (green)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_3) * 0); // 0% duty cycle for PWM7 (blue)
    
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT | PWM_OUT_5_BIT, true); // Enable PWM output channels
    
    break;
  case 2:  //Blue
    
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);
    PWMGenEnable(PWM0_BASE, PWM_GEN_3);
    
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 0); // 0% duty cycle for PWM4 (red)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 0); // 0% duty cycle for PWM5 (green)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_3) * 1); // 100% duty cycle for PWM7 (blue)
    
    PWMOutputState(PWM0_BASE, PWM_OUT_7_BIT, true); // Enable PWM output channels
    
    break;
    
  case 3:  //Green
    
  PWMGenEnable(PWM0_BASE, PWM_GEN_2);
  
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 0); // 0% duty cycle for PWM4 (red)
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2) * 1); // 100% duty cycle for PWM5 (green)
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_7, PWMGenPeriodGet(PWM0_BASE, PWM_GEN_3) * 0); // 0% duty cycle for PWM7 (blue)
  
  PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, true); // Enable PWM output channels
  break;
  default:
    break;
}
}
void LEDturnOFF(void)
{ 
    //PWMGenDisable(PWM0_BASE, PWM_GEN_2); // Disable PWM GEN2
    //PWMGenDisable(PWM0_BASE, PWM_GEN_3); // Disable PWM GEN3
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT | PWM_OUT_5_BIT | PWM_OUT_7_BIT, false); // Disable PWM output channels
}

void blinkLED(Color c, int seconds, int turns)
{
 
  for(int i = 0; i < turns; i++)
  {
    LEDturnON(c);
    delay_s(seconds);
    LEDturnOFF();
    delay_s(seconds);
  }
}