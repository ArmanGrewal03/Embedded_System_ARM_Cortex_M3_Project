#include <stdio.h>
#include <stdint.h>

#include "LPC17xx.h"
#include "system_LPC17xx.h"
#include "GLCD.h"
#include "KBD.h"

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbaudio.h"
#include "type.h"

#include "player.h"

/* These globals mirror Keil USB audio example usage */
extern void SystemClockUpdate(void);
extern uint32_t SystemFrequency;

uint8_t  Mute;
uint32_t Volume;
static  uint8_t quit = 0;

#if USB_DMA
uint32_t *InfoBuf = (uint32_t *)(DMA_BUF_ADR);
short    *DataBuf = (short *)(DMA_BUF_ADR + 4*P_C);
#else
uint32_t InfoBuf[P_C];
short    DataBuf[B_S];
#endif

uint16_t DataOut;
uint16_t DataIn;
uint8_t  DataRun;
uint16_t PotVal;
uint32_t VUM;
uint32_t Tick;

extern uint16_t VolCur;

/* Read potentiometer into PotVal (matches Keil example flow) */
static void get_potval(void){
  uint32_t val;
  LPC_ADC->ADCR |= 0x01000000;
  do { val = LPC_ADC->ADGDR; } while ((val & 0x80000000) == 0);
  LPC_ADC->ADCR &= ~0x01000000;
  PotVal = ((val >> 8) & 0xF8) + ((val >> 7) & 0x08);
}

/* Lightweight on-screen HUD for volume; called from ISR */
static uint8_t s_last_vol = 255;
static void media_update_volume_ui(uint8_t vol_percent){
  if (vol_percent == s_last_vol) return;
  s_last_vol = vol_percent;

  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
  GLCD_DisplayString(2, 0, 1, (unsigned char*)"USB Audio: CONNECTED        ");
  GLCD_DisplayString(3, 0, 1, (unsigned char*)"Volume (pot):               ");

  char buf[16];
  sprintf(buf, "%3u%%", vol_percent);
  GLCD_DisplayString(3, 16, 1, (unsigned char*)buf);

  GLCD_Bargraph(20, 5*16, 280, 16, vol_percent);
}

/* Timer ISR: feeds DAC with samples and updates volume/VU */
void TIMER0_IRQHandler(void){
  long      val;
  uint32_t  cnt;

  if (DataRun) {
    val = DataBuf[DataOut];
    cnt = (DataIn - DataOut) & (B_S - 1);
    if (cnt == (B_S - P_C*P_S)) { DataOut++; }
    if (cnt > (P_C*P_S))        { DataOut++; }
    DataOut &= B_S - 1;

    if (val < 0) VUM -= val; else VUM += val;

    val  *= Volume;
    val >>= 16;
    val  += 0x8000;
    val  &= 0xFFFF;
  } else {
    val = 0x8000;
  }

  if (Mute) { val = 0x8000; }

  LPC_DAC->DACR = val & 0xFFC0;

  if ((Tick++ & 0x03FF) == 0) {
    get_potval();
    if (VolCur == 0x8000) { Volume = 0; }
    else { Volume = VolCur * PotVal; }

    long vu = (long)(VUM >> 20);
    VUM = 0;
    if (vu > 7) vu = 7;

    uint32_t pot_raw = PotVal & 0x3FF;
    uint8_t  vol_pct = (uint8_t)((pot_raw * 100U) / 1023U);
    media_update_volume_ui(vol_pct);
  }

  LPC_TIM0->IR = 1;

  if (get_button() & KBD_SELECT){
    NVIC_DisableIRQ(TIMER0_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
    quit = 1;
  }
}

void runMusicPlayer(void){
  GLCD_Clear(White);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_DisplayString(0, 0, 1, (unsigned char*)"  MP3 Player (USB) ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
  GLCD_DisplayString(1, 0, 1, (unsigned char*)"Press SELECT to stop and return");

  volatile uint32_t pclkdiv, pclk;
  SystemClockUpdate();

  LPC_PINCON->PINSEL1 &= ~((0x03<<18)|(0x03<<20));
  LPC_PINCON->PINSEL1 |=  ((0x01<<18)|(0x02<<20));

  LPC_SC->PCONP |= (1 << 12);
  LPC_ADC->ADCR  = 0x00200E04;
  LPC_DAC->DACR  = 0x00008000;

  pclkdiv = (LPC_SC->PCLKSEL0 >> 2) & 0x03;
  switch (pclkdiv){
    case 0x01: pclk = SystemFrequency;    break;
    case 0x02: pclk = SystemFrequency/2;  break;
    case 0x03: pclk = SystemFrequency/8;  break;
    case 0x00:
    default:   pclk = SystemFrequency/4;  break;
  }

  LPC_TIM0->MR0 = pclk/DATA_FREQ - 1;
  LPC_TIM0->MCR = 3;
  LPC_TIM0->TCR = 1;
  NVIC_EnableIRQ(TIMER0_IRQn);

  USB_Init();
  USB_Connect(TRUE);

  quit = 0;
  while(1){
    if (quit){ quit = 0; return; }
  }
}
