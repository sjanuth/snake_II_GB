#include <gb/gb.h>
#include <stdint.h>
#include "splash_bg_asset.h"
#include "sprites/snake_head_down.h"
#include "sprites/snake_head_right.h"
#include "sprites/snake_tail_down.h"
#include "sprites/snake_tail_right.h"

uint8_t joypadCurrent = 0, joypadPrevious = 0;
typedef enum {
  UP,
  RIGHT,
  DOWN,
  LEFT,
  NONVALID
}direction_type;

enum{
  SNAKE_HEAD_DOWN_TILE,
  SNAKE_HEAD_RIGHT_TILE,
  SNAKE_TAIL_DOWN_TILE,
  SNAKE_TAIL_RIGHT_TILE
};

direction_type snake_direction = RIGHT;
direction_type dpad_direction = NONVALID;
/*  number of segments between head and tail */
uint8_t snake_length = 0;
uint8_t score = 0;

int main(void)
{
  DISPLAY_ON;
  SHOW_BKG;
  SPRITES_8x8;
  // Load & set our background data
  set_bkg_data(0, splash_bg_asset_TILE_COUNT, splash_bg_asset_tiles);

  // The gameboy screen is 160px wide by 144px tall
  // We deal with tiles that are 8px wide and 8px tall
  // 160/8 = 20 and 120/8=15
  set_bkg_tiles(0, 1, 20, 15, splash_bg_asset_map);

  /*  wait on splash screen until start button is pressed */
#if 1
    while(!(joypad() & J_START)){vsync();}
#else
    delay(1000);
#endif

  HIDE_BKG;
  SPRITES_8x8;
  SHOW_SPRITES;

  set_sprite_data(SNAKE_HEAD_DOWN_TILE, 1, snake_head_down);
  set_sprite_data(SNAKE_HEAD_RIGHT_TILE, 1, snake_head_right);
  set_sprite_tile(0, SNAKE_HEAD_DOWN_TILE);
  uint8_t i = 0;
  for (i = 1; i < 160/8; i++){
    set_sprite_tile(i, 0);
    /*  max 10 sprites per line */
    move_sprite(i,  8 *i , 16);

  }

  set_sprite_prop(0, S_FLIPY);

  int8_t velocity = 1;

  // Set the sprite's default position
  uint8_t spriteX = 80;
  uint8_t spriteY = 72;
  uint8_t movement_pending = 0;
  /*  gets called at 60fps  */
  uint16_t update_screen_counter = 0;

  // Loop forever
  while (1) {

    joypadPrevious=joypadCurrent;
    joypadCurrent = joypad();

    if(joypadCurrent & J_UP){dpad_direction = UP;}
    else if((joypadCurrent & J_RIGHT)){dpad_direction = RIGHT;}
    else if((joypadCurrent & J_DOWN)){dpad_direction = DOWN;}
    else if((joypadCurrent & J_LEFT)){dpad_direction = LEFT;}

    if (dpad_direction != NONVALID && !movement_pending){
      if (((dpad_direction + snake_direction) % 2) == 1){
        /*  Neither in current direction or opposite */
        snake_direction = dpad_direction;
        movement_pending = 1;
        dpad_direction = NONVALID;
      }
    }


    if(update_screen_counter % (60/velocity) == 0){

      // Apply our velocity
      switch(snake_direction){
        case UP:
          spriteY -=8;
          break;
        case LEFT:
          spriteX -=8;
          break;
        case DOWN:
          spriteY +=8;
          break;
        case RIGHT:
          spriteX +=8;
          break;
      }
      movement_pending = 0;

    if (spriteX > 160 ){spriteX = 4; }
    else if(spriteX < 8){spriteX = 160;}
    else if(spriteY < 16){spriteY = 144;}
    else if(spriteY > 144){spriteY = 16;}
      // Position the first sprite at our spriteX and spriteY
      // All sprites are render 8 pixels to the left of their x position and 16
      // pixels ABOVE their actual y position This means an object rendered at 0,0
      // will not be visible x+5 and y+12 will center the 8x8 tile at our x and y
      // position
        move_sprite(0, spriteX , spriteY );
    }

    update_screen_counter++;
    // Done processing, yield CPU and wait for start of next frame
    vsync();
  }
  return 0;
}
