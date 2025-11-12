#include <stdio.h>
#include <stdint.h>
#include "GLCD.h"
#include "KBD.h"

#include "gallery.h"
#include "cat.h"
#include "tree.h"

#define IMG0_W    CAT_WIDTH
#define IMG0_H    CAT_HEIGHT
#define IMG0_DATA CAT_PIXEL_DATA

#define IMG1_W    TREE_WIDTH
#define IMG1_H    TREE_HEIGHT
#define IMG1_DATA TREE_PIXEL_DATA

static void ui_header(const char* title){
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_ClearLn(0, 1);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)title);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

static int gallery_index = 0;
static int gallery_count = 0;

static void gallery_draw_current(void){
  GLCD_Clear(White);
  ui_header("Photo Gallery   <SELECT to Back>");

  int gallery_counter = 0;

  #if defined(IMG0_DATA)
  if (gallery_index == gallery_counter++) {
      GLCD_Bitmap(0, 0, IMG0_W, IMG0_H, (const unsigned char*)IMG0_DATA);
  }
  #endif

  #if defined(IMG1_DATA)
  if (gallery_index == gallery_counter++) {
      GLCD_Bitmap(0, 0, IMG1_W, IMG1_H, (const unsigned char*)IMG1_DATA);
  }
  #endif

  GLCD_SetTextColor(Black);
  GLCD_DisplayString(9, 0, 1, (unsigned char*)"LEFT/RIGHT: Prev/Next   SELECT: Back");
}

void Gallery(void){
  #if defined(IMG1_DATA)
    gallery_count = 2;
  #elif defined(IMG0_DATA)
    gallery_count = 1;
  #else
    gallery_count = 1; /* stub page */
  #endif

  int last = -1;
  gallery_draw_current();

  while(1){
    uint32_t key = get_button();
    if (key & KBD_SELECT) return;
    if (key & KBD_LEFT)  gallery_index--;
    if (key & KBD_RIGHT) gallery_index++;

    if (gallery_index < 0)               gallery_index = gallery_count-1;
    if (gallery_index >= gallery_count)  gallery_index = 0;

    if (last != gallery_index){
      last = gallery_index;
      gallery_draw_current();
    }
    for (volatile uint32_t d=0; d<40000; d++) __NOP();
  }
}
