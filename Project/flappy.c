#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "GLCD.h"
#include "KBD.h"
#include "flappy.h"

#define SW 320
#define SH 240

#ifndef __FI
#define __FI 1
#endif

/* Game params */
#define BIRD_X         40
#define BIRD_SIZE      10
#define GRAVITY        1    
#define FLAP_VEL      -7      
#define MAX_FALL       8
#define PIPE_W         36
#define PIPE_GAP       60
#define PIPE_SPEED      2
#define PIPE_SPACING  120      

typedef struct {
  int16_t x;
  int16_t gap_y;  
  uint8_t active;
} Pipe;

static int16_t bird_y, bird_v;
static int score;

static void draw_rect(int x, int y, int w, int h, unsigned short bg, unsigned short fg, int fill100){
  GLCD_SetBackColor(bg);
  GLCD_SetTextColor(fg);
  GLCD_Bargraph(x, y, w, h, fill100 ? 100 : 0);
}

static void draw_bird(void){
  draw_rect(BIRD_X, bird_y, BIRD_SIZE, BIRD_SIZE, White, Red, 1);
}

static void clear_bird(void){
  draw_rect(BIRD_X, bird_y, BIRD_SIZE, BIRD_SIZE, White, White, 1);
}

static void draw_pipe(const Pipe* p){
  int top_h = p->gap_y - PIPE_GAP/2;
  int bot_y = p->gap_y + PIPE_GAP/2;
  int bot_h = SH - bot_y;
  if (top_h < 0) top_h = 0;
  if (bot_h < 0) bot_h = 0;
  /* top segment */
  draw_rect(p->x, 0, PIPE_W, top_h, White, Green, 1);
  /* bottom segment */
  draw_rect(p->x, bot_y, PIPE_W, bot_h, White, Green, 1);
}

static void clear_pipe(const Pipe* p){
  int top_h = p->gap_y - PIPE_GAP/2;
  int bot_y = p->gap_y + PIPE_GAP/2;
  int bot_h = SH - bot_y;
  if (top_h < 0) top_h = 0;
  if (bot_h < 0) bot_h = 0;
  draw_rect(p->x, 0, PIPE_W, top_h, White, White, 1);
  draw_rect(p->x, bot_y, PIPE_W, bot_h, White, White, 1);
}

static int collide(const Pipe* p){
  if (BIRD_X + BIRD_SIZE <= p->x) return 0;
  if (BIRD_X >= p->x + PIPE_W)    return 0;
  int gap_top = p->gap_y - PIPE_GAP/2;
  int gap_bot = p->gap_y + PIPE_GAP/2;
  int bird_top = bird_y;
  int bird_bot = bird_y + BIRD_SIZE;
  if (bird_top < gap_top || bird_bot > gap_bot) return 1;
  return 0;
}

static void hud_draw(void){
  char buf[32];
  GLCD_SetBackColor(Blue);
  GLCD_SetTextColor(Yellow);
  GLCD_ClearLn(0,1);
  snprintf(buf, sizeof(buf), " Flappy  Score:%d ", score);
  GLCD_DisplayString(0, 0, __FI, (unsigned char*)buf);
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Black);
}

static void wait_short(void){
  for (volatile uint32_t d=0; d<25000; d++) __NOP();
}

static void game_over_screen(void){
  GLCD_SetBackColor(White);
  GLCD_SetTextColor(Red);
  GLCD_DisplayString(4, 4, __FI, (unsigned char*)" GAME OVER ");
  GLCD_SetTextColor(Black);
  GLCD_DisplayString(6, 1, __FI, (unsigned char*)"SELECT: Restart  LEFT: Exit");
}

void Flappy_Run(void){
  Pipe pipes[2];

  while (1){
    GLCD_Clear(White);
    score  = 0;
    bird_y = SH/2 - BIRD_SIZE/2;
    bird_v = 0;

    pipes[0].x = SW + 10;
    pipes[0].gap_y = 60 + (rand()% (SH-120));
    pipes[0].active = 1;

    pipes[1].x = pipes[0].x + PIPE_SPACING;
    pipes[1].gap_y = 60 + (rand()% (SH-120));
    pipes[1].active = 1;

    hud_draw();

    int alive = 1;

    while (alive){
      uint32_t k = get_button();
      if (k & KBD_SELECT) { bird_v = FLAP_VEL; } /* flap */
      if (k & KBD_LEFT)   { return; }            /* exit to caller (menu) */

      /* physics */
      bird_v += GRAVITY; if (bird_v > MAX_FALL) bird_v = MAX_FALL;
      clear_bird();
      bird_y += bird_v;
      if (bird_y < 16) bird_y = 16;               /* keep header line clear */
      if (bird_y + BIRD_SIZE >= SH) { alive = 0; }

      /* move & redraw pipes */
      for (int i=0;i<2;i++){
        clear_pipe(&pipes[i]);
        pipes[i].x -= PIPE_SPEED;
        if (pipes[i].x + PIPE_W < 0){
          pipes[i].x = SW + PIPE_SPACING;
          pipes[i].gap_y = 60 + (rand()% (SH-120));
          pipes[i].active = 1;
        }
        draw_pipe(&pipes[i]);
      }

      for (int i=0;i<2;i++){
        if (collide(&pipes[i])) { alive = 0; }
        if (pipes[i].active && (pipes[i].x + PIPE_W) < BIRD_X){
          pipes[i].active = 0; score++;
          hud_draw();
        }
      }

      draw_bird();
      wait_short();
    }

    game_over_screen();

    while (1){
      uint32_t k = get_button();
      if (k & KBD_SELECT) break; 
      if (k & KBD_LEFT)   return; 
      wait_short();
    }
  }
}
