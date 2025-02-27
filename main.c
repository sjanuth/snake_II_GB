#include "main.h"
#include "snake.h"
#include "snake_bckg.h"
#include "snake_bckg_tileset.h"
#include "sprites/food.h"
#include "splash_bg_asset.h"
#include <gb/gb.h>
#include <stdint.h>
#include <rand.h>

/* Definitions and globals variables */

#define OPPOSITE_DIRECTION(X) ((X + 2) % 4)
/*  2 tiles for score, 1 tile for border, 1 tile for grid */
#define PLAYFIELD_Y_OFFSET (4)
/*  The plafield has the coordinates
 *  x = [0 -17]
 *  y = [0- 12]*/
#define PLAYFIELD_TO_SPRITE_X_POS(X) ((X*8) + 16)
#define PLAYFIELD_TO_SPRITE_Y_POS(Y) ((4 * 8) + 16 + (8 * Y))
#define PLAYFIELD_TO_GLOBAL_X_POS(X) (X + 1) /* border */
#define PLAYFIELD_TO_GLOBAL_Y_POS(Y) (Y + PLAYFIELD_Y_OFFSET) /* borders + 2 for score */
#define PLAYFIELD_X_MAX (17)
#define PLAYFIELD_Y_MAX (12)
#define BACKGROUND_EMPTY_TILE 11
#define GBP_FPS 60
#define STARTPOS_X ((160 / 8) / 2)
#define STARTPOS_Y ((144 / 8) / 2)
#define FOOD_SPRITE 0


/*  Since the GB has very limited RAM, using heap will lead to fragmented
 * memory. Thus, we use a memory pool for nodes */
snake_node_t node_pool[MAX_NODES];

uint16_t score = 0;

pos_t get_random_free_food_position(snake_t *snake){

  pos_t random_pos;
  do{
    /* TODO: This will become an issue when the snake grows longer.
     * It might take too much time to randomly find a free spot and cause lag.
     * Better implement a list with all free positions (numerated from 0 to max_size of all availble fields and draw a single random number */
#if 0
    /*  This will be faster but more deterministic */
    random_pos.x = DIV_REG % PLAYFIELD_X_MAX;
    random_pos.y = DIV_REG % PLAYFIELD_Y_MAX;
#else
    random_pos.x = rand() % PLAYFIELD_X_MAX;
    random_pos.y= rand() % PLAYFIELD_Y_MAX;

#endif
  }
  while(checkPointForCollision(snake, PLAYFIELD_TO_GLOBAL_X_POS(random_pos.x), PLAYFIELD_TO_GLOBAL_Y_POS(random_pos.y)));

  return random_pos;
}

void main(void) {

  uint8_t joypadCurrent = 0, joypadPrevious = 0;
  direction_type snake_direction = RIGHT;
  direction_type dpad_direction = RIGHT;

  uint8_t velocity = 2;
  uint8_t has_food_in_mouth = 0;
  pos_t food_pos;

  DISPLAY_ON;
  SHOW_BKG;
  SHOW_SPRITES;
  SPRITES_8x8;

  set_bkg_data(0, splash_bg_asset_TILE_COUNT, splash_bg_asset_tiles);

  /* The gameboy screen is 160px wide by 144px tall
   * We deal with tiles that are 8px wide and 8px tall
   * 160/8 = 20 and 120/8=15 */

  set_bkg_tiles(0, 1, 20, 15, splash_bg_asset_map);

  /*  wait on splash screen until start button is pressed */

#if 1
  while (!(joypad() & (J_START | J_A | J_B))) {
    vsync();
  }
#else
  delay(1000);
#endif

  initrand(DIV_REG); /* Seed with the Game Boy's divider register */

  /*  reload background tiles for main game */
  set_bkg_data(0, 32, snake_bckg_tileset);
  /*  main game background with borders */
  set_bkg_tiles(0, 0, 20, 18, snake_bckg);

  set_sprite_data(FOOD_SPRITE, 1, food);
  set_sprite_prop(FOOD_SPRITE, 0);

  /*  Instantiate our snake object and create nodes */

  snake_node_t *snake_head = allocateNode();
  if (snake_head) {
    snake_head->x_pos = STARTPOS_X;
    snake_head->y_pos = STARTPOS_Y;
    snake_head->prev_node = NULL;
    snake_head->next_node = NULL;
    snake_head->tile_to_render = SNAKE_HEAD_RIGHT_TILE;
    snake_head->dir_to_next_node = LEFT;
  };

  snake_node_t *snake_first_node = allocateNode();
  if (snake_first_node) {
    snake_first_node->x_pos = STARTPOS_X - 1;
    snake_first_node->y_pos = STARTPOS_Y;
    snake_first_node->prev_node = snake_head;
    snake_first_node->next_node = NULL;
    snake_first_node->tile_to_render = SNAKE_BODY_LEFT_RIGHT;
    snake_first_node->dir_to_next_node = LEFT;
  };

  snake_head->next_node = snake_first_node;

  snake_node_t *snake_second_node = allocateNode();
  if (snake_second_node) {
    snake_second_node->x_pos = STARTPOS_X - 2;
    snake_second_node->y_pos = STARTPOS_Y;
    snake_second_node->prev_node = snake_first_node;
    snake_second_node->next_node = NULL;
    snake_second_node->tile_to_render = SNAKE_BODY_LEFT_RIGHT;
    snake_second_node->dir_to_next_node = LEFT;
  };

  snake_first_node->next_node = snake_second_node;

  snake_node_t *snake_tail = allocateNode();
  if (snake_tail) {
    snake_tail->x_pos = STARTPOS_X - 3;
    snake_tail->y_pos = STARTPOS_Y;
    snake_tail->prev_node = snake_second_node;
    snake_tail->next_node = NULL;
    snake_tail->tile_to_render = SNAKE_TAIL_RIGHT;
  };

  snake_second_node->next_node = snake_tail;

  snake_t snake = {
      .length = 4,
      .head = snake_head,
      .tail = snake_tail,
  };

  /*  render the snake for the first time */

  snake_node_t *node_to_render = snake.head;
  while (node_to_render != NULL) {
    set_bkg_tile_xy(node_to_render->x_pos, node_to_render->y_pos,
                    node_to_render->tile_to_render);
    node_to_render = node_to_render->next_node;
  }

  /*  randomly place food on a free spot */

  pos_t rand_pos;
  rand_pos = get_random_free_food_position(&snake);

  food_pos.x = rand_pos.x;
  food_pos.y = rand_pos.y;

  move_sprite(FOOD_SPRITE, (uint8_t)PLAYFIELD_TO_SPRITE_X_POS(food_pos.x), (uint8_t) PLAYFIELD_TO_SPRITE_Y_POS(food_pos.y));

  /*  Flag to check if a movement is pending to get rendered before catching a
   * new one */

  uint8_t movement_pending = 0;

  /*  Gets called at 60fps  */

  uint16_t update_screen_counter = 0;

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

      /*  Before moving the snake tile in background, we must clear the current
       * tile */

      uint8_t tmp_x = snake.head->x_pos;
      uint8_t tmp_y = snake.head->y_pos;

      set_bkg_tile_xy(tmp_x, tmp_y, BACKGROUND_EMPTY_TILE);

      /*  Instead of moving and updating every node in the linked list,
       *  we just insert a new node after the head and remove the node before
       * the tail.*/

      snake_node_t *new_node = allocateNode();
      if (new_node) {
        new_node->x_pos = tmp_x;
        new_node->y_pos = tmp_y;
        new_node->dir_to_next_node = snake.head->dir_to_next_node;
        new_node->tile_to_render = BACKGROUND_EMPTY_TILE;
        new_node->next_node = snake.head->next_node;
        new_node->prev_node = snake.head;
      }

      /* find which snake body tile must get rendered */

      direction_type dir_n = snake.head->dir_to_next_node;

      /*  direction no next node is opposite to current moving direction */

      direction_type opposite_dir = OPPOSITE_DIRECTION(snake_direction);
      snake.head->dir_to_next_node = opposite_dir;
      if(has_food_in_mouth){
        /*  TODO: up, down, left and right don't share the same tile */
        if (snake_direction == UP && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_FOOD_EATEN;
        } else if (snake_direction == RIGHT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_RIGHT_DOWN;
        } else if (snake_direction == LEFT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_DOWN_LEFT;
        }

        else if (snake_direction == UP && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_UP_RIGTH;
        } else if (snake_direction == LEFT && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_FOOD_EATEN;
        } else if (snake_direction == DOWN && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_RIGHT_DOWN;
        }

        else if (snake_direction == UP && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_FOOD_EATEN;
        } else if (snake_direction == DOWN && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_DOWN_LEFT;
        }

        else if (snake_direction == LEFT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_UP_RIGTH;
        } else if (snake_direction == DOWN && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_FOOD_EATEN;
        }
      }

      else{
        /* normal without food eaten */
        if (snake_direction == UP && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_UP_DOWN;
        } else if (snake_direction == RIGHT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_RIGHT_DOWN;
        } else if (snake_direction == LEFT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_DOWN_LEFT;
        }

        else if (snake_direction == UP && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_UP_RIGHT;
        } else if (snake_direction == LEFT && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_LEFT_RIGHT;
        } else if (snake_direction == DOWN && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_RIGHT_DOWN;
        }

        else if (snake_direction == UP && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_LEFT_RIGHT;
        } else if (snake_direction == DOWN && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_DOWN_LEFT;
        }

        else if (snake_direction == LEFT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_UP_RIGHT;
        } else if (snake_direction == DOWN && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_UP_DOWN;
        }
      }

      /* inject new node */

      snake.head->next_node->prev_node = new_node;
      snake.head->next_node = new_node;

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

      if (snake.head->x_pos > PLAYFIELD_WIDTH) {
        snake.head->x_pos = 1;
      } else if (snake.head->x_pos < 1) {
        snake.head->x_pos = PLAYFIELD_WIDTH;
      } else if (snake.head->y_pos < PLAYFIELD_Y_OFFSET) {
        snake.head->y_pos = PLAYFIELD_HEIGHT;
      } else if (snake.head->y_pos > PLAYFIELD_HEIGHT) {
        snake.head->y_pos = PLAYFIELD_Y_OFFSET;
      }

      /*  Check if food is eaten */

      if (checkPointForCollision(&snake, PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x), PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y))){
        has_food_in_mouth = 1;

        pos_t rand_pos;
        rand_pos = get_random_free_food_position(&snake);

        food_pos.x = rand_pos.x;
        food_pos.y = rand_pos.y;

        move_sprite(FOOD_SPRITE, (uint8_t)PLAYFIELD_TO_SPRITE_X_POS(food_pos.x), (uint8_t) PLAYFIELD_TO_SPRITE_Y_POS(food_pos.y));
      }else{
        has_food_in_mouth = 0;
      }

      /*  Update tail and remove node before tail */

      set_bkg_tile_xy(snake.tail->x_pos, snake.tail->y_pos,
                      BACKGROUND_EMPTY_TILE);
      snake_node_t *second_last_node = snake.tail->prev_node;
      snake.tail->x_pos = second_last_node->x_pos;
      snake.tail->y_pos = second_last_node->y_pos;

      direction_type new_tail_direction =
          OPPOSITE_DIRECTION(second_last_node->prev_node->dir_to_next_node);
      snake.tail->tile_to_render = SNAKE_TAIL_UP + new_tail_direction;

      second_last_node->prev_node->next_node = snake.tail;
      snake.tail->prev_node = second_last_node->prev_node;

      second_last_node->prev_node = NULL;
      second_last_node->next_node = NULL;
      freeNode(second_last_node);

      /*  Toggle the flag and indicate that we have rendered the tile and can
       * process a new direction */

      if (movement_pending) {
        movement_pending = 0;
      }

      /* Check if the next field in moving direction is food. 
       * If so, then render the snake with the mouth open */

      //TODO: what if the food is on the other side of the border?
      // still render the normal tile or mouth open tile?
      // Keep it simple for now and just render mouth open if not wrapped around

      uint8_t food_lies_ahead = 0;
      switch (snake_direction) {
        case UP:
          if (snake_head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){break;};
          if(snake_head->y_pos - 1 == PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){
            food_lies_ahead = 1;
          }
          break;
        case RIGHT:
          if (snake_head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){break;};
          if(snake_head->x_pos + 1 == PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){
            food_lies_ahead = 1;
          }
          break;
        case DOWN:
          if (snake_head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){break;};
          if(snake_head->y_pos + 1 == PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){
            food_lies_ahead = 1;
          }
          break;
        case LEFT:
          if (snake_head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){break;};
          if(snake_head->x_pos - 1 == PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){
            food_lies_ahead = 1;
          }
      }

      
      /*  Update background tiles */

      if(food_lies_ahead){
        set_bkg_tile_xy(snake.head->x_pos, snake.head->y_pos, snake_direction + SNAKE_MOUTH_OPEN_UP);
      }else{
        set_bkg_tile_xy(snake.head->x_pos, snake.head->y_pos, snake_direction);
      }
      snake_node_t *node_after_head = snake.head->next_node;
      set_bkg_tile_xy(node_after_head->x_pos, node_after_head->y_pos,
                      node_after_head->tile_to_render);
      set_bkg_tile_xy(snake.tail->x_pos, snake.tail->y_pos,
                      snake.tail->tile_to_render);
    }
    update_screen_counter++;

    /* Done processing, yield CPU and wait for start of next frame */
    vsync();
  }
}
