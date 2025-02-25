#include "snake_bckg.h"
#include "snake_bckg_tileset.h"
#include "splash_bg_asset.h"
#include <gb/gb.h>
#include <stdint.h>

typedef enum {
  UP,
  RIGHT,
  DOWN,
  LEFT,
} direction_type;

#define BACKGROUND_EMPTY_TILE 11
#define GBP_FPS 60

enum {
  SNAKE_HEAD_UP_TILE,
  SNAKE_HEAD_RIGHT_TILE,
  SNAKE_HEAD_DOWN_TILE,
  SNAKE_HEAD_LEFT_TILE
};

/*  number of segments between head and tail */
#define DEFAULT_LENGTH_START 5
uint8_t snake_length = DEFAULT_LENGTH_START;
uint8_t score = 0;

int main(void) {

  uint8_t joypadCurrent = 0, joypadPrevious = 0;
  direction_type snake_direction = RIGHT;
  direction_type dpad_direction = RIGHT;
  uint8_t snake_head_tile_to_render_on_bkg = SNAKE_HEAD_RIGHT_TILE;

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
  while (!(joypad() & J_START)) {
    vsync();
  }
#else
  delay(1000);
#endif

  /*  reload background tiles */
  set_bkg_data(0, 12, snake_bckg_tileset);
  /*  main game background with borders */
  set_bkg_tiles(0, 0, 20, 18, snake_bckg);

  int8_t velocity = 2;

  // Set the sprite's default position
  uint8_t spriteX = (160 / 8) / 2;
  uint8_t spriteY = (144 / 8) / 2;

  /*  flag to check if a movement is pending to get rendered
   *  before catching a new one */
  uint8_t movement_pending = 0;

  /*  gets called at 60fps  */
  uint16_t update_screen_counter = 0;

  // Loop forever
  while (1) {

    joypadPrevious = joypadCurrent;
    joypadCurrent = joypad();

    if (joypadCurrent & J_UP) {
      dpad_direction = UP;
    } else if ((joypadCurrent & J_RIGHT)) {
      dpad_direction = RIGHT;
    } else if ((joypadCurrent & J_DOWN)) {
      dpad_direction = DOWN;
    } else if ((joypadCurrent & J_LEFT)) {
      dpad_direction = LEFT;
    }

    if (!movement_pending) {
      if (((dpad_direction + snake_direction) % 2) == 1) {
        /*  Neither in current direction or opposite */
        snake_direction = dpad_direction;
        movement_pending = 1;
      }
    }

    if (update_screen_counter % (GBP_FPS / velocity) == 0) {

      /*  before moving the snake tile in background, we must clear the current
       * tile */
      set_bkg_tile_xy(spriteX, spriteY, BACKGROUND_EMPTY_TILE);

      // Apply our velocity
      switch (snake_direction) {
      case UP:
        spriteY -= 1;
        break;
      case LEFT:
        spriteX -= 1;
        break;
      case DOWN:
        spriteY += 1;
        break;
      case RIGHT:
        spriteX += 1;
        break;
      }

      if (movement_pending) {
        snake_head_tile_to_render_on_bkg = snake_direction;
        movement_pending = 0;
      }

      /* Check if snake has touched the borders  */
      if (spriteX > 20 - 1 - 1) {
        spriteX = 1;
      } else if (spriteX < 1) {
        spriteX = 20 - 1 - 1;
      } else if (spriteY < 4) {
        spriteY = 18 - 1 - 1;
      } else if (spriteY > 18 - 1 - 1) {
        spriteY = 4;
      }

      set_bkg_tile_xy(spriteX, spriteY, snake_head_tile_to_render_on_bkg);
    }
    update_screen_counter++;

    // Done processing, yield CPU and wait for start of next frame
    vsync();
  }
  return 0;
}
