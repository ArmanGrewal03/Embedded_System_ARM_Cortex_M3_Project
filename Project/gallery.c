#include <stdio.h>
#include <stdint.h>
#include "GLCD.h"
#include "KBD.h"
#define __FI 1

#include "gallery.h"
#include "cat.h"
#include "tree.h"
#include "leafs.c"

#define IMG0_W    CAT_WIDTH
#define IMG0_H    CAT_HEIGHT
#define IMG0_DATA CAT_PIXEL_DATA

#define IMG1_W    TREE_WIDTH
#define IMG1_H    TREE_HEIGHT
#define IMG1_DATA TREE_PIXEL_DATA

#define IMG2_W    LEAFS_WIDTH
#define IMG2_H    LEAFS_HEIGHT
#define IMG2_DATA LEAFS_pixel_data

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

  int gallery_counter = 0;

  #if defined(IMG0_DATA)
  if (gallery_index == gallery_counter++) {
      GLCD_Bitmap(0, 0, IMG0_W, IMG0_H, (unsigned char*)IMG0_DATA);
  }
  #endif

  #if defined(IMG1_DATA)
  if (gallery_index == gallery_counter++) {
      GLCD_Bitmap(0, 0, IMG1_W, IMG1_H, (unsigned char*)IMG1_DATA);
  }
  #endif
	#if defined(IMG2_DATA)
  if (gallery_index == gallery_counter++) {
      GLCD_Bitmap(60, 0, IMG2_W, IMG2_H, (unsigned char*)IMG2_DATA);
  }
  #endif
	ui_header("Photo Gallery   <SELECT to Back>");
  GLCD_SetTextColor(Black);
  GLCD_DisplayString(9, 0, 1, (unsigned char*)"LEFT/RIGHT: Prev/Next   SELECT: Back");
}

void Gallery(void){
	#if defined(IMG2_DATA)
	    gallery_count = 3;
  #elif defined(IMG1_DATA)
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
