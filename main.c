#include <gb/gb.h>
#include <stdint.h>
#include "splash_bg_asset.h"
#include "snake_head.h"


void main(void)
{
  DISPLAY_ON;
  SHOW_BKG;

  // Load & set our background data
  set_bkg_data(0, splash_bg_asset_TILE_COUNT, splash_bg_asset_tiles);

  // The gameboy screen is 160px wide by 144px tall
  // We deal with tiles that are 8px wide and 8px tall
  // 160/8 = 20 and 120/8=15
  set_bkg_tiles(0, 1, 20, 15, splash_bg_asset_map);
  delay(1000);

  HIDE_BKG;
  SHOW_SPRITES;
  set_sprite_data(0, 1, snake_head);
  set_sprite_tile(0, 0);
  //move_sprite(0, 84, 88);
  set_sprite_prop(0, 0b01000000); /* vertical flip sprite */

  uint8_t spriteX, spriteY;
  int8_t velocityX, velocityY;

  // Set the sprite's default position
  spriteX = 80;
  spriteY = 72;

  // Set our default velocity to be down and to the right
  velocityX = 1;
  velocityY = 1;

  // Loop forever
  while (1) {

    // Game main loop processing goes here
    // Apply our velocity
    spriteX += velocityX;
    //spriteY += velocityY;

    // When we get too far to the right while moving to the right
    if (spriteX > 160 && velocityX > 0) {

      // Switch directions for our x velocity
      //velocityX = -velocityX;

      // Clamp our x position back down to 156 so we don't go past the edge
      spriteX = 4;
    }

    // When we get too far down on the screen while moving downards
    if (spriteY > 140 && velocityY > 0) {

      // Switch directions for our y velocity
      velocityY = -velocityY;

      // Clamp our y position back down to 140 so we don't go past the edge
      spriteY = 140;
    }

    // When we get too far to the left while moving left
    if (spriteX < 4 && velocityX < 0) {

      // Switch directions for our x velocity
      velocityX = -velocityX;

      // Clamp our x position back up to 4 so we don't go past the edge
      spriteX = 4;
    }

    // When we get too far towards the top of the screen while moving upwards
    if (spriteY < 4 && velocityY < 0) {

      // Switch directions for our y velocity
      velocityY = -velocityY;

      // Clamp our y position back up to 4 so we don't go past the edge
      spriteY = 4;
    }

    // Position the first sprite at our spriteX and spriteY
    // All sprites are render 8 pixels to the left of their x position and 16
    // pixels ABOVE their actual y position This means an object rendered at 0,0
    // will not be visible x+5 and y+12 will center the 8x8 tile at our x and y
    // position
    move_sprite(0, spriteX + 4, spriteY + 12);
    // Done processing, yield CPU and wait for start of next frame
    vsync();
  }
}
