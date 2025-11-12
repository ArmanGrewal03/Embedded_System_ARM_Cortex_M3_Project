#include "GLCD.h"
#define __FI 1

#include "LPC17xx.h"
#include "system_LPC17xx.h"
#include <stdio.h>
#include <stdint.h>

#include "Board_LED.h"
#include "KBD.h"

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbaudio.h"
#include "type.h"

#include "cat.h"
#include "tree.h"

#include "menu.h"  

int main (void) {
  LED_Initialize();
  GLCD_Init();
  KBD_Init();

  GLCD_Clear(White);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)"     COE718 Final Project   ");
  GLCD_SetTextColor(White);
  GLCD_DisplayString(1, 0, __FI, (unsigned char*)"     Arman Grewal   ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Red);

  initMenu();
  selectMenu(0);
  runMenu();
}
