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

#define __FI 1

extern void SystemClockUpdate(void);
extern uint32_t SystemFrequency;

/* --- Audio globals expected by Keil USB audio stack (like usbdmain.c) --- */
uint8_t  Mute;          /* Mute state */
uint32_t Volume;        /* Volume level used in ISR */

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

extern uint16_t VolCur; /* from adcuser.c */

/* latest volume percentage for UI (written in ISR, read in main loop) */
static volatile uint8_t ui_vol_pct = 0;

/* ---------- UI helpers (match style of Gallery) ---------- */

static void ui_header(const char* title){
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_ClearLn(0, 1);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)title);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

static uint8_t last_vol_pct = 255;

static void draw_volume_ui(uint8_t vol_pct){
  if (vol_pct == last_vol_pct) return;
  last_vol_pct = vol_pct;

  /* Title/header line */
  ui_header("MP3 Player   <SELECT to Back>");

  /* Info + bar graph */
  GLCD_DisplayString(2, 0, 1, (unsigned char*)"USB Audio: CONNECTED        ");
  GLCD_DisplayString(3, 0, 1, (unsigned char*)"Volume (pot):               ");

  char buf[16];
  sprintf(buf, "%3u%%", vol_pct);
  GLCD_DisplayString(3, 16, 1, (unsigned char*)buf);

  GLCD_Bargraph(20, 5*16, 280, 16, vol_pct);

  GLCD_DisplayString(9, 0, 1,
    (unsigned char*)"Use POT for volume   SELECT: Back");
}

/* ---------- Potentiometer read (AIN2 on P0.25) ---------- */

static void get_potval(void){
  uint32_t val;

  /* start A/D conversion */
  LPC_ADC->ADCR |= 0x01000000;
  do {
    val = LPC_ADC->ADGDR;                 /* read global data register */
  } while ((val & 0x80000000) == 0);      /* wait end of conversion    */
  LPC_ADC->ADCR &= ~0x01000000;           /* stop conversion           */

  PotVal = ((val >> 8) & 0xF8) +          /* extract potentiometer     */
           ((val >> 7) & 0x08);
}

/* ---------- Timer0 ISR: audio + pot/volume (NO GLCD here) ---------- */

void TIMER0_IRQHandler(void){
  long      val;
  uint32_t  cnt;

  if (DataRun) {
    val = DataBuf[DataOut];                 /* get audio sample           */
    cnt = (DataIn - DataOut) & (B_S - 1);   /* buffer fill level          */

    if (cnt == (B_S - P_C*P_S)) { DataOut++; }  /* too much data, skip one */
    if (cnt >  (P_C*P_S))       { DataOut++; }  /* still enough data       */
    DataOut &= B_S - 1;

    if (val < 0) VUM -= val; else VUM += val;   /* accumulate for VU meter */

    val  *= Volume;                        /* apply volume               */
    val >>= 16;
    val  += 0x8000;                        /* add bias                   */
    val  &= 0xFFFF;
  } else {
    val = 0x8000;                          /* DAC mid point              */
  }

  if (Mute) {
    val = 0x8000;
  }

  /* write to DAC (using DACR from your header) */
  LPC_DAC->DACR = val & 0xFFC0;

  /* every 1024th tick (~31 Hz @ 32 kHz sample rate):
     update pot, volume, and export a UI volume percentage */
  if ((Tick++ & 0x03FF) == 0) {
    get_potval();

    /* chained USB volume (VolCur) and pot (PotVal) */
    if (VolCur == 0x8000) {        /* minimum level (mute) */
      Volume = 0;
    } else {
      Volume = VolCur * PotVal;
    }

    long vu = (long)(VUM >> 20);   /* VU meter scale (not drawn here, but kept) */
    VUM = 0;
    if (vu > 7) vu = 7;

    /* map pot (0..~255) to 0..100% for bar graph */
    uint32_t pot_raw = PotVal & 0x3FF;  /* mask to 10 bits just in case */
    ui_vol_pct = (uint8_t)((pot_raw * 100U) / 1023U);
  }

  LPC_TIM0->IR = 1;                /* clear interrupt flag */
}

/* ---------- Main "MP3 player" mode (Gallery-style joystick) ---------- */

void runMusicPlayer(void){
  volatile uint32_t pclkdiv, pclk;

  GLCD_Clear(White);
  ui_header("MP3 Player   <SELECT to Back>");

  GLCD_DisplayString(1, 0, 1,
    (unsigned char*)"Waiting for USB audio stream...");
  GLCD_DisplayString(9, 0, 1,
    (unsigned char*)"Use POT for volume   SELECT: Back");

  /* System clock setup */
  SystemClockUpdate();

  /* P0.25 = AD0.2 (pot), P0.26 = AOUT (DAC) */
  LPC_PINCON->PINSEL1 &= ~((0x03<<18)|(0x03<<20));
  LPC_PINCON->PINSEL1 |=  ((0x01<<18)|(0x02<<20));

  /* enable ADC peripheral clock */
  LPC_SC->PCONP |= (1 << 12);

  /* ADC: 10-bit, AIN2 @ 4MHz (matches Keil demo settings) */
  LPC_ADC->ADCR  = 0x00200E04;
  /* DAC mid point */
  LPC_DAC->DACR  = 0x00008000;

  /* derive PCLK for TIMER0 from SystemFrequency */
  pclkdiv = (LPC_SC->PCLKSEL0 >> 2) & 0x03;
  switch (pclkdiv){
    case 0x01: pclk = SystemFrequency;    break;
    case 0x02: pclk = SystemFrequency/2;  break;
    case 0x03: pclk = SystemFrequency/8;  break;
    case 0x00:
    default:   pclk = SystemFrequency/4;  break;
  }

  /* 32 kHz audio sample rate (DATA_FREQ from usbaudio.h) */
  LPC_TIM0->MR0 = pclk/DATA_FREQ - 1;
  LPC_TIM0->MCR = 3;                      /* interrupt + reset on MR0   */
  LPC_TIM0->TCR = 1;                      /* enable timer               */
  NVIC_EnableIRQ(TIMER0_IRQn);

  /* USB audio device init */
  USB_Init();
  USB_Connect(TRUE);

  /* initial UI */
  draw_volume_ui(0);

  int last_vol_drawn = -1;

  /* ---- Joystick loop: replicate Gallery() behavior ---- */
  while (1){
    uint32_t key = get_button();

    /* CENTER CLICK (SELECT) = exit + disconnect, just like Gallery() */
    if (key & KBD_SELECT) {
      NVIC_DisableIRQ(TIMER0_IRQn);
      NVIC_DisableIRQ(USB_IRQn);
      USB_Connect(FALSE);

      GLCD_Clear(White);  /* optional: clear screen on exit */
      return;             /* back to menu */
    }

    /* update the volume bar from the latest value coming from the ISR */
    uint8_t vol_now = ui_vol_pct;
    if (vol_now != last_vol_drawn){
      last_vol_drawn = vol_now;
      draw_volume_ui(vol_now);
    }

    /* small debounce / update delay (similar to Gallery) */
    for (volatile uint32_t d = 0; d < 40000; d++) {
      __NOP();
    }
  }
}