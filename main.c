#include "main.h"
#include "snake.h"
#include "snake_bckg.h"
#include "snake_bckg_tileset.h"
#include "sprites/food.h"
#include "splash_bg_asset.h"
#include <gb/gb.h>
#include <stdint.h>
#include <rand.h>
#include <stdio.h>
#include "vwf.h"
#include "vwf_font.h"
#include "animals.h"
#include "animals_tiles.h"

/* Definitions and globals variables */

#define OPPOSITE_DIRECTION(X) ((X + 2) % 4)
/*  2 tiles for score, 1 tile for border, 1 tile for grid */
#define PLAYFIELD_Y_OFFSET (4)
/*  The playfield has the coordinates
 *  x = [0 -17]
 *  y = [0- 12]*/
#define PLAYFIELD_TO_SPRITE_X_POS(X) ((X*8) + 16)
#define PLAYFIELD_TO_SPRITE_Y_POS(Y) ((4 * 8) + 16 + (8 * Y))
#define PLAYFIELD_TO_GLOBAL_X_POS(X) (X + 1) /* border */
#define PLAYFIELD_TO_GLOBAL_Y_POS(Y) (Y + PLAYFIELD_Y_OFFSET) /* borders + 2 for score */
#define PLAYFIELD_X_MAX (17)
#define PLAYFIELD_Y_MAX (12)
#define GBP_FPS 60
#define STARTPOS_X ((160 / 8) / 2)
#define STARTPOS_Y ((144 / 8) / 2)

uint8_t joypadCurrent = 0, joypadPrevious = 0;

/*  Since the GB has very limited RAM, using heap will lead to fragmented
 * memory. Thus, we use a memory pool for nodes */
snake_node_t node_pool[MAX_NODES];


pos_t get_random_free_food_position(snake_t *snake){

  pos_t random_pos;

#if 0
  /*  for testing */
  random_pos.x = 0;
  random_pos.y = PLAYFIELD_Y_MAX ;
  return random_pos;
#endif

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
  while(checkPointForCollision(
        snake,
        PLAYFIELD_TO_GLOBAL_X_POS(random_pos.x),
        PLAYFIELD_TO_GLOBAL_Y_POS(random_pos.y))
      );

  return random_pos;
}

/**
 * Because sprintf does not support padding with %0xd in GBDK,
 * we have to do the zero padding ourselves
 *
 * @param buffer String buffer where the result will be stored
 * @param number Pointer to the number to be converted
 * @param width How long the zero padded number should be
 */
void int_to_str_padded(char *buffer, uint16_t *number, uint8_t width) {
    uint16_t temp = *number;
    uint8_t digits = 0;

    /* Count digits in the number */
    do {
        digits++;
        temp /= 10;
    } while (temp > 0);

    uint8_t padding = width - digits;
    uint8_t i = 0;

    while (padding-- > 0) {
        buffer[i++] = '0';
    }

    sprintf(buffer + i, "%d", *number);
}

void render_score(uint16_t *score){
  char str_score[5];
  int_to_str_padded(str_score, score, 4);
  vwf_draw_text(1, 1, 60, (unsigned char *) str_score);
}

uint8_t is_button_pressed_debounced(uint8_t mask){
  return (joypadCurrent & mask) && !(joypadPrevious & mask);
}

void wait_until_pressed_debounced(uint8_t mask){
  joypadPrevious = joypadCurrent;
  joypadCurrent = joypad();
  while (!(joypadCurrent & mask ) || (joypadPrevious & mask)) {
    vsync();
    joypadPrevious = joypadCurrent;
    joypadCurrent = joypad();
  }
}

void main(void) {

  direction_type snake_direction;
  direction_type dpad_direction;

  uint16_t score;
  uint8_t velocity = 3;
  uint8_t has_food_in_mouth = 0;
  /* Bring up an animal after 5 meals (animals don't count here) */
  uint8_t food_counter = 0;
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
  wait_until_pressed_debounced(J_START | J_A | J_B);
#else
  delay(1000);
#endif

  initrand(DIV_REG); /* Seed with the Game Boy's divider register */

  /*  reload background tiles for main game */
  set_bkg_data(0, 51, snake_bckg_tileset);

#define TILE_SIZE (16) /*  8x8 Bits and 2 bit color */
  set_sprite_data(FOOD_SPRITE, 1, food);
  set_sprite_data(SPIDER_SPRITE, 2, &animals_tiles[0]);
  set_sprite_data(MOUSE_SPRITE , 2, &animals_tiles[ANIMAL_MOUSE_1 * TILE_SIZE]);
  set_sprite_data(FISH_SPRITE , 2, &animals_tiles[ANIMAL_FISH_1 * TILE_SIZE]);
  set_sprite_data(BUG_SPRITE , 2, &animals_tiles[ANIMAL_BUG_1 * TILE_SIZE]);
  set_sprite_data(TURTLE_SPRITE, 2, &animals_tiles[ANIMAL_TURTLE_1 * TILE_SIZE]);
  set_sprite_data(ANT_SPRITE, 2, &animals_tiles[ANIMAL_ANT_1 * TILE_SIZE]);

  /*  Load fonts */

  vwf_load_font(0, vwf_font, BANK(vwf_font));

  vwf_activate_font(0);
  vwf_set_destination(VWF_RENDER_BKG);

GameStart:

  /*  main game background with borders */
  set_bkg_tiles(0, 0, 20, 18, snake_bckg);

  score = 0;
  render_score(&score);

  snake_direction = RIGHT;
  dpad_direction = RIGHT;

  uint16_t i;
  for(i = 0; i < MAX_NODES; i++){
    freeNode(&node_pool[i]);
  }

#if 1
  // quick test draw metasprite
  move_metasprite_ex(animal_spider_metasprite,SPIDER_SPRITE,0,SPIDER_SPRITE,
      PLAYFIELD_TO_SPRITE_X_POS(0),PLAYFIELD_TO_SPRITE_Y_POS(0));
  move_metasprite_ex(animal_mouse_metasprite,MOUSE_SPRITE ,0,MOUSE_SPRITE,
     PLAYFIELD_TO_SPRITE_X_POS(0),PLAYFIELD_TO_SPRITE_Y_POS(1));
  move_metasprite_ex(animal_fish_metasprite,FISH_SPRITE ,0,FISH_SPRITE,
     PLAYFIELD_TO_SPRITE_X_POS(0),PLAYFIELD_TO_SPRITE_Y_POS(2));
  move_metasprite_ex(animal_bug_metasprite,BUG_SPRITE ,0, BUG_SPRITE,
     PLAYFIELD_TO_SPRITE_X_POS(0),PLAYFIELD_TO_SPRITE_Y_POS(3));
  move_metasprite_ex(animal_turtle_metasprite,TURTLE_SPRITE, 0, TURTLE_SPRITE,
     PLAYFIELD_TO_SPRITE_X_POS(0),PLAYFIELD_TO_SPRITE_Y_POS(4));
  move_metasprite_ex(animal_ant_metasprite,ANT_SPRITE ,0, ANT_SPRITE,
     PLAYFIELD_TO_SPRITE_X_POS(0),PLAYFIELD_TO_SPRITE_Y_POS(5));
#endif

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
    snake_first_node->tile_to_render = SNAKE_BODY_STRAIGHT_RIGHT;
    snake_first_node->dir_to_next_node = LEFT;
  };

  snake_head->next_node = snake_first_node;

  snake_node_t *snake_second_node = allocateNode();
  if (snake_second_node) {
    snake_second_node->x_pos = STARTPOS_X - 2;
    snake_second_node->y_pos = STARTPOS_Y;
    snake_second_node->prev_node = snake_first_node;
    snake_second_node->next_node = NULL;
    snake_second_node->tile_to_render = SNAKE_BODY_STRAIGHT_RIGHT;
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

    if ((joypadCurrent & J_START) && !(joypadPrevious & J_START)){

      /*  Take a snapshot of the current background */
      uint8_t background_data_snake[20*18];
      wait_vbl_done();  // Wait for VBlank before reading
      get_bkg_tiles(0, 0, 20, 18, &background_data_snake[0]);

      /*  Just show Borders without score or snake */
      set_bkg_tiles(0, 0, 20, 18, snake_bckg);
      vwf_draw_text(STARTPOS_X - 2, STARTPOS_Y, 70, "Paused");
      render_score(&score);
      vsync();

      wait_until_pressed_debounced(J_START);
      set_bkg_tiles(0, 0, 20, 18, background_data_snake);
      vsync();
      delay(400);
    }

    if (is_button_pressed_debounced(J_UP)) {
      dpad_direction = UP;
    } else if (is_button_pressed_debounced(J_RIGHT)) {
      dpad_direction = RIGHT;
    } else if (is_button_pressed_debounced(J_DOWN)) {
      dpad_direction = DOWN;
    } else if (is_button_pressed_debounced(J_LEFT)) {
      dpad_direction = LEFT;
    } else if (is_button_pressed_debounced(J_A)) {
      /* Turn right in moving direction */
      dpad_direction = (snake_direction + 1) % 4;
    } else if (is_button_pressed_debounced(J_B)) {
      /* Turn left in moving direction */
      dpad_direction = (snake_direction + 3) % 4;
    }

    if (!movement_pending) {
      if (((dpad_direction + snake_direction) % 2) == 1) {
        /*  Neither in current direction or opposite */
        snake_direction = dpad_direction;
        movement_pending = 1;
      }
    }

    if (update_screen_counter % (GBP_FPS / velocity) == 0) {


      /*  Instead of moving and updating every node in the linked list,
       *  we just insert a new node after the head and remove the node before
       * the tail.*/

      snake_node_t *new_node = allocateNode();
      if (new_node) {
        new_node->x_pos = snake.head->x_pos;
        new_node->y_pos = snake.head->y_pos;
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
        if (snake_direction == UP && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_UP;
        } else if (snake_direction == RIGHT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_RIGHT_DOWN;
        } else if (snake_direction == LEFT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_DOWN_LEFT;
        }

        else if (snake_direction == UP && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_UP_RIGTH;
        } else if (snake_direction == LEFT && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_LEFT;
        } else if (snake_direction == DOWN && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_RIGHT_DOWN;
        }

        else if (snake_direction == UP && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_RIGHT;
        } else if (snake_direction == DOWN && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_DOWN_LEFT;
        }

        else if (snake_direction == LEFT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_UP_RIGTH;
        } else if (snake_direction == DOWN && dir_n == UP) {
          new_node->tile_to_render = SNAKE_FOOD_EATEN_DOWN;
        }
      }

      else{
        /* normal without food eaten */
        if (snake_direction == UP && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_STRAIGHT_UP;
        } else if (snake_direction == RIGHT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_RIGHT_DOWN;
        } else if (snake_direction == LEFT && dir_n == DOWN) {
          new_node->tile_to_render = SNAKE_BODY_DOWN_LEFT;
        }

        else if (snake_direction == UP && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_UP_RIGHT;
        } else if (snake_direction == LEFT && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_STRAIGHT_LEFT;
        } else if (snake_direction == DOWN && dir_n == RIGHT) {
          new_node->tile_to_render = SNAKE_BODY_RIGHT_DOWN;
        }

        else if (snake_direction == UP && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_STRAIGHT_RIGHT;
        } else if (snake_direction == DOWN && dir_n == LEFT) {
          new_node->tile_to_render = SNAKE_BODY_DOWN_LEFT;
        }

        else if (snake_direction == LEFT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_LEFT_UP;
        } else if (snake_direction == RIGHT && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_UP_RIGHT;
        } else if (snake_direction == DOWN && dir_n == UP) {
          new_node->tile_to_render = SNAKE_BODY_STRAIGHT_DOWN;
        }
      }

      /* inject new node */

      snake.head->next_node->prev_node = new_node;
      snake.head->next_node = new_node;

      /* update coordinates for the head */

      pos_t anticipated_next_pos = {
        .x = snake.head->x_pos,
        .y = snake.head->y_pos,
      };

      switch (snake_direction) {
      case UP:
        anticipated_next_pos.y = snake.head->y_pos - 1;
        break;
      case LEFT:
        anticipated_next_pos.x = snake.head->x_pos - 1;
        break;
      case DOWN:
        anticipated_next_pos.y = snake.head->y_pos + 1;
        break;
      case RIGHT:
        anticipated_next_pos.x = snake.head->x_pos + 1;
        break;
      }

      /* Check if next position touches the borders and a wrap around occurs */

      if (anticipated_next_pos.x > PLAYFIELD_WIDTH) {
         anticipated_next_pos.x = 1;
      } else if (anticipated_next_pos.x < 1) {
         anticipated_next_pos.x = PLAYFIELD_WIDTH;
      } else if (anticipated_next_pos.y < PLAYFIELD_Y_OFFSET) {
        anticipated_next_pos.y = PLAYFIELD_HEIGHT;
      } else if (anticipated_next_pos.y > PLAYFIELD_HEIGHT) {
        anticipated_next_pos.y = PLAYFIELD_Y_OFFSET;
      }

      /*  Check if snake will bite itself */

      if(checkPointForCollision(&snake, anticipated_next_pos.x, anticipated_next_pos.y)){

        /*  Edge case: if the snake will bite in the tail, the game will continue */

        if(!((snake.tail->x_pos == anticipated_next_pos.x) && (snake.tail->y_pos == anticipated_next_pos.y))){
#define flash_interval (200) /* TODO: measure actual interval */

          uint8_t background_data_snake[20*18];
          wait_vbl_done();  // Wait for VBlank before reading
          get_bkg_tiles(0, 0, 20, 18, &background_data_snake[0]);
          uint8_t i;
          for(i = 0; i < 5; i++){
            /*  Clear snake from screen, just show borders */
            set_bkg_tiles(0, 0, 20, 18, snake_bckg);
            vsync();
            delay(flash_interval);

            /* show snake on background */
            set_bkg_tiles(0, 0, 20, 18, background_data_snake);
            vsync();
            delay(flash_interval);
          }

          /*  TODO: If a new high score was reached, display a notification */

          wait_until_pressed_debounced(J_START | J_A );

          goto GameStart;
        }
      }

      set_bkg_tile_xy(snake.head->x_pos, snake.head->y_pos, BACKGROUND_EMPTY_TILE);

      snake.head->y_pos = anticipated_next_pos.y;
      snake.head->x_pos = anticipated_next_pos.x;

      /*  Update tail and remove node before tail */

      if(!has_food_in_mouth){
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
      }

      /*  Check if food is eaten */

      if (checkPointForCollision(&snake, PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x), PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y))){
        has_food_in_mouth = 1;
        score += velocity;
        render_score(&score);

        food_counter++;
        if (food_counter >= 5 ){
          /*  Time to show an animal */
          food_counter = 0;
          //TODO: show animal
        }

        pos_t rand_pos;
        rand_pos = get_random_free_food_position(&snake);

        food_pos.x = rand_pos.x;
        food_pos.y = rand_pos.y;

        move_sprite(FOOD_SPRITE, (uint8_t)PLAYFIELD_TO_SPRITE_X_POS(food_pos.x), (uint8_t) PLAYFIELD_TO_SPRITE_Y_POS(food_pos.y));
      }else{
        has_food_in_mouth = 0;
      }

      /*  Toggle the flag and indicate that we have rendered the tile and can
       * process a new direction */

      if (movement_pending) {
        movement_pending = 0;
      }

      /* Check if the next field in moving direction is food.
       * If so, then render the snake with the mouth open */

      uint8_t food_lies_ahead = 0;
      switch (snake_direction) {
        case UP:
          if (snake_head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){break;};
          if(snake_head->y_pos - 1 == PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){
            food_lies_ahead = 1;
          }
          /* Check wrapped around position */
          else if(PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y) == PLAYFIELD_HEIGHT){
             if(snake_head->y_pos - 1 < PLAYFIELD_Y_OFFSET ){
               food_lies_ahead = 1;
             }
          }
         break;
        case RIGHT:
          if (snake_head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){break;};
          if(snake_head->x_pos + 1 == PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){
            food_lies_ahead = 1;
          }
          /* Check wrapped around position */
          else if(PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x) == 1){
              if(snake_head->x_pos + 1 > PLAYFIELD_WIDTH ){
                food_lies_ahead = 1;
              }
          }
          break;
        case DOWN:
          if (snake_head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){break;};
          if(snake_head->y_pos + 1 == PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){
            food_lies_ahead = 1;
          }
          /* Check wrapped around position */
          else if(PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y) == PLAYFIELD_Y_OFFSET){
              if(snake_head->y_pos + 1 > PLAYFIELD_HEIGHT ){
                food_lies_ahead = 1;
              }
          }
          break;
        case LEFT:
          if (snake_head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)){break;};
          if(snake_head->x_pos - 1 == PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)){
            food_lies_ahead = 1;
          }
          /* Check wrapped around position */
          else if(PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x) == PLAYFIELD_WIDTH){
              if(snake_head->x_pos -1 < 1 ){
                food_lies_ahead = 1;
              }
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
