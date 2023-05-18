#ifndef _RGB_H_
#define _RGB_H_

//RGB LED related function definitions

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
void blinkLED(Color c, float seconds, int turns);

#endif