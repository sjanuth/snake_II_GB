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
#define STARTPOS_X ((160 / 8) / 2)
#define STARTPOS_Y ((144 / 8) / 2)

enum {
  SNAKE_HEAD_UP_TILE,
  SNAKE_HEAD_RIGHT_TILE,
  SNAKE_HEAD_DOWN_TILE,
  SNAKE_HEAD_LEFT_TILE
};

enum {
  SNAKE_BODY_UP_DOWN = 12,
  SNAKE_BODY_LEFT_RIGHT,
  SNAKE_BODY_UP_RIGHT,
  SNAKE_BODY_RIGHT_DOWN,
  SNAKE_BODY_DOWN_LEFT,
  SNAKE_BODY_LEFT_UP,
  SNAKE_BODY_FOOD_EATEN,
  SNAKE_FOOD,
  SNAKE_TAIL_RIGHT,
  SNAKE_TAIL_DOWN,
  SNAKE_TAIL_LEFT,
  SNAKE_TAIL_UP,
  SNAKE_MOUTH_OPEN_RIGHT,
  SNAKE_MOUTH_OPEN_DOWN,
  SNAKE_MOUTH_OPEN_LEFT,
  SNAKE_MOUTH_OPEN_UP,
};

typedef struct snake_node_s{
  uint8_t tile_to_render;
  uint8_t x_pos;
  uint8_t y_pos;
  struct snake_node_s * next_node;
  struct snake_node_s * prev_node;
  direction_type dir_to_next_node;
}snake_node_t;

typedef struct snake_s{
  snake_node_t * head;
  snake_node_t * tail;
  uint8_t length;
}snake_t;

uint16_t score = 0;

void main(void) {

  uint8_t joypadCurrent = 0, joypadPrevious = 0;
  direction_type snake_direction = RIGHT;
  direction_type dpad_direction = RIGHT;

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
  set_bkg_data(0, 28, snake_bckg_tileset);
  /*  main game background with borders */
  set_bkg_tiles(0, 0, 20, 18, snake_bckg);

  int8_t velocity = 2;

  /*  Instantiate our snake object and create nodes */

  snake_node_t snake_head = {
    .x_pos = STARTPOS_X,
    .y_pos = STARTPOS_Y,
    .prev_node = NULL,
    .next_node = NULL,
    .tile_to_render = SNAKE_HEAD_RIGHT_TILE,
    .dir_to_next_node = LEFT,
  };

  snake_node_t snake_first_node = {
    .x_pos = STARTPOS_X - 1,
    .y_pos = STARTPOS_Y,
    .prev_node = &snake_head,
    .next_node = NULL,
    .tile_to_render = SNAKE_BODY_LEFT_RIGHT,
    .dir_to_next_node = LEFT,
  };

  snake_head.next_node = &snake_first_node;

  snake_node_t snake_second_node = {
    .x_pos = STARTPOS_X - 2,
    .y_pos = STARTPOS_Y,
    .prev_node = &snake_first_node,
    .next_node = NULL,
    .tile_to_render = SNAKE_BODY_LEFT_RIGHT,
    .dir_to_next_node = LEFT,
  };

  snake_first_node.next_node = &snake_second_node;

  snake_node_t snake_tail = {
    .x_pos = STARTPOS_X - 3,
    .y_pos = STARTPOS_Y,
    .prev_node = &snake_second_node,
    .next_node = NULL,
    .tile_to_render = SNAKE_TAIL_RIGHT,
  };

  snake_second_node.next_node = &snake_tail;

  snake_t snake = {
    .length = 4,
    .head = &snake_head,
    .tail = &snake_tail,
  };

  /*  render the snake for the first time */

  snake_node_t * node_to_render = snake.head;
  while (node_to_render != NULL) {
      set_bkg_tile_xy(node_to_render->x_pos, node_to_render->y_pos, node_to_render->tile_to_render);
      node_to_render = node_to_render->next_node;
  }

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

      /*  before moving the snake tile in background, we must clear the current tile */

      uint8_t tmp_x = snake.head->x_pos;
      uint8_t tmp_y = snake.head->y_pos;

      set_bkg_tile_xy(tmp_x, tmp_y, BACKGROUND_EMPTY_TILE);

      /*  instead of moving and updating every node in the linked list,
       *  we just insert a new node after the head and remove the node before
       *  the tail.*/

      snake_node_t new_node = {
        .x_pos = tmp_x,
        .y_pos = tmp_y,
        .tile_to_render = BACKGROUND_EMPTY_TILE,
        .next_node = snake.head->next_node,
        .prev_node = snake.head
      };

      /* find which snake body tile must get rendered */
      direction_type dir_n = snake.head->dir_to_next_node;

      /*  direction no next node is opposite to current moving direction */
      direction_type opposite_dir = (snake_direction + 2) % 4;
      snake.head->dir_to_next_node = opposite_dir;

      if (snake_direction == UP && dir_n == DOWN){
        new_node.tile_to_render = SNAKE_BODY_UP_DOWN;
      }
      else if (snake_direction == RIGHT && dir_n == DOWN){
        new_node.tile_to_render = SNAKE_BODY_RIGHT_DOWN;
      }
      else if (snake_direction == LEFT && dir_n == DOWN){
        new_node.tile_to_render = SNAKE_BODY_DOWN_LEFT;
      }


      else if (snake_direction == UP && dir_n == RIGHT){
        new_node.tile_to_render = SNAKE_BODY_UP_RIGHT;
      }
      else if (snake_direction == LEFT && dir_n == RIGHT){
        new_node.tile_to_render = SNAKE_BODY_LEFT_RIGHT;
      }
      else if (snake_direction == DOWN && dir_n == RIGHT){
        new_node.tile_to_render = SNAKE_BODY_RIGHT_DOWN;
      }


      else if (snake_direction == UP && dir_n == LEFT){
        new_node.tile_to_render = SNAKE_BODY_LEFT_UP;
      }
      else if (snake_direction == RIGHT && dir_n == LEFT){
        new_node.tile_to_render = SNAKE_BODY_LEFT_RIGHT;
      }
      else if (snake_direction == DOWN && dir_n == LEFT){
        new_node.tile_to_render = SNAKE_BODY_DOWN_LEFT;
      }

      else if (snake_direction == LEFT && dir_n == UP){
        new_node.tile_to_render = SNAKE_BODY_LEFT_UP;
      }
      else if (snake_direction == RIGHT && dir_n == UP){
        new_node.tile_to_render = SNAKE_BODY_UP_RIGHT;
      }
      else if (snake_direction == DOWN && dir_n == UP){
        new_node.tile_to_render = SNAKE_BODY_UP_DOWN;
      }

      /* inject new node */
      snake.head->next_node->prev_node = &new_node;
      snake.head->next_node = &new_node;

      /* update coordinates for the head */

      switch (snake_direction) {
      case UP:
        snake.head->y_pos -= 1;
        break;
      case LEFT:
        snake.head->x_pos -= 1;
        break;
      case DOWN:
        snake.head->y_pos += 1;
        break;
      case RIGHT:
        snake.head->x_pos += 1;
        break;
      }

      /* Check if snake has touched the borders  */

      if (snake.head->x_pos > 20 - 1 - 1) {
        snake.head->x_pos = 1;
      } else if (snake.head->x_pos < 1) {
        snake.head->x_pos = 20 - 1 - 1;
      } else if (snake.head->y_pos < 4) {
        snake.head->y_pos = 18 - 1 - 1;
      } else if (snake.head->y_pos > 18 - 1 - 1) {
        snake.head->y_pos = 4;
      }

      /*  toggle the flag and indicate that we have rendered the tile and can process a new direction */

      if (movement_pending) {
        movement_pending = 0;
      }

      set_bkg_tile_xy(snake.head->x_pos, snake.head->y_pos, snake_direction);
      snake_node_t *node_after_head = snake.head->next_node;
      set_bkg_tile_xy(node_after_head->x_pos, node_after_head->y_pos, node_after_head->tile_to_render);
    }
    update_screen_counter++;

    /* Done processing, yield CPU and wait for start of next frame */
    vsync();
  }
}
