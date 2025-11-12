#include <stdio.h>
#include <stdint.h>
#include "GLCD.h"
#include "KBD.h"

#include "menu.h"
#include "gallery.h"
#include "player.h"

static void menu_footer(int selected, int total){
  char f[32];
  snprintf(f, sizeof(f), "UP/DOWN  SELECT   %d/%d", selected + 1, total);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
  GLCD_ClearLn(8, 1);
  GLCD_DisplayString(8, 0, 1, (unsigned char*)f);
}

void initMenu(void){
  GLCD_Clear(White);
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(White);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)"     Menu    ");
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

void selectMenu(int selected){
  const int total = 2;
  for (int i = 0; i < total; i++){
    GLCD_SetBackColor(White);
    GLCD_SetTextColor(Black);
    GLCD_ClearLn(2 + i, 1);

    const char *label = (i == 0) ? "Photo Gallery" : "MP3 Player";
    char line[32];
    snprintf(line, sizeof(line), "%c %s", (i == selected) ? '>' : ' ', label);

    if (i == selected) { GLCD_SetBackColor(Green); GLCD_SetTextColor(Black); }
    else               { GLCD_SetBackColor(White); GLCD_SetTextColor(Black); }

    GLCD_DisplayString(2 + i, 0, __FI, (unsigned char*)line);
  }
  menu_footer(selected, total);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

void runMenu(void){
  int selected = 0;
  selectMenu(selected);
  while(1){
    uint32_t key = get_button();
    if (key & KBD_UP)    { if (selected > 0) selected--; selectMenu(selected); }
    else if (key & KBD_DOWN){ if (selected < 1) selected++; selectMenu(selected); }
    else if (key & KBD_SELECT) {
      GLCD_Clear(White);
      GLCD_SetTextColor(Black);
      GLCD_DisplayString(1, 0, __FI, (unsigned char*)"     Loading    ");
      switch (selected) {
        case 0: Gallery();        initMenu(); selectMenu(selected); break;
        case 1: runMusicPlayer(); initMenu(); selectMenu(selected); break;
      }
    }
    for (volatile uint32_t d=0; d<40000; d++) __NOP();
  }
}
