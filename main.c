#include "animals.h"
#include "animals_tiles.h"
#include "gb/gb.h"
#include "gb/hardware.h"
#include "main.h"
#include "savefile.h"
#include "snake.h"
#include "snake_bckg.h"
#include "snake_bckg_tileset.h"
#include "snake_main_site.h"
#include "sound.h"
#include "sprites/food.h"
#include "vwf.h"
#include "vwf_font.h"
#include <rand.h>
#include <stdint.h>
#include <stdio.h>

/* Definitions and globals variables */

#define TILE_SIZE (16) /*  8x8 Bits and 2 bit color */
#define FLASH_INTERVAL ((1600 / 4) / 2)
#define NB_OF_DIFFERENT_ANIMALS 6
#define MAX_LEVEL (9)
#define MIN_LEVEL (1)
#define START_DIRECTION (UP)

/*  The playfield has the coordinates
 *  x = [0 -17]
 *  y = [0- 12]
 */
#define PLAYFIELD_X_MAX (17)
#define PLAYFIELD_Y_MAX (12)
#define PLAYFIELD_SIZE ((PLAYFIELD_X_MAX + 1) * (PLAYFIELD_Y_MAX + 1))
#define GBP_FPS 60
#define STEP_POS_X ((160 / 8) - 2)
#define STEP_POS_Y (1)
#define NO_FREE_FIELDS_AVAILABLE (255)
#define FOOD_NOT_SET (255)
#define ANIMAL_TOPBAR_X_POS (160 - (2 * 8) - 8 - (2 * 8))
#define ANIMAL_TOPBAR_Y_POS (16 + 8)

uint8_t joypadCurrent = 0, joypadPrevious = 0;
pos_t food_pos;
pos_t animal_pos = {.x = FOOD_NOT_SET, .y = FOOD_NOT_SET};
uint8_t step_counter; /*  required for scoring when animal appears */
uint8_t shown_animal_type;
const uint8_t distribution_required_food_for_animals[] = {5, 5, 5, 5, 5,
                                                          5, 6, 6, 6, 7};

const uint16_t velocity_rates_ms_per_step[] = { 500, 450, 400, 350, 300, 250, 200, 150, 100 };
/*  The GB has a LCD refresh rate of 60fps -> 0.0166s
 *  This lookup table contain  the counter values that must get reached to do a step */
const uint8_t velocity_rates_counter_values[] = {30, 27, 24, 21, 18, 15, 12, 9, 6};


extern snake_node_t node_pool[MAX_NODES];

/* Function definitions */

/**
 * @brief Check if stored data is available by checking
 * to a predefined value SAVECHECK_VALUE
 */
static uint8_t HasExistingSave(void) {

  uint8_t saveDataExists = 0;

  ENABLE_RAM;

  saveDataExists = saved_check_flag == SAVECHECK_VALUE;

  DISABLE_RAM;
  return saveDataExists;
}

/**
 * @brief Copy top score from the cartridge's FRAM/SRAM
 * into the normal on-device RAM
 */
static void LoadSavedData(uint16_t *top_score, uint8_t *velocity) {
  ENABLE_RAM;
  *top_score = top_score_save;
  *velocity = velocity_save;
  DISABLE_RAM;
}

/**
 * @brief Save current top score from RAM into FRAM/SRAM
 */
static void SaveData(uint16_t *top_score, uint8_t *velocity) {
  ENABLE_RAM;
  saved_check_flag = SAVECHECK_VALUE;
  top_score_save = *top_score;
  velocity_save = *velocity;
  DISABLE_RAM;
}

/**
 * Get random position to place food on playfield
 * @param *snake
 * @return random availble position
 * */
static pos_t get_random_free_food_position(snake_t *snake, uint8_t is_for_animal) {

  uint8_t x_upper_limit = is_for_animal ? PLAYFIELD_X_MAX - 1 : PLAYFIELD_X_MAX;
  uint8_t x, y;

  // Try a limited number of random positions before falling back to a full scan
  uint8_t attempts = 50; // Avoids infinite loops if the field is almost full
  while (attempts--) {
    x = rand() % (x_upper_limit + 1);
    y = rand() % (PLAYFIELD_Y_MAX + 1);

    // Skip food position check (faster than iterating over all)
    if (food_pos.x == x && food_pos.y == y)
      continue;

    // Check animal position (if applicable)
    if (animal_pos.x != FOOD_NOT_SET) {
      if ((animal_pos.x == x && animal_pos.y == y) ||
          (animal_pos.x + 1 == x && animal_pos.y == y)) {
        continue;
      }
    }

    // Check collision with the snake
    if (!checkPointForCollision(snake, PLAYFIELD_TO_GLOBAL_X_POS(x),
                                PLAYFIELD_TO_GLOBAL_Y_POS(y))) {
      pos_t result = {.x = x, .y = y};
      return result; // Return first valid position
    }
  }

  // If no random spot found, fall back to slower full scan
  for (x = 0; x <= x_upper_limit; x++) {
    for (y = 0; y <= PLAYFIELD_Y_MAX; y++) {
      if (food_pos.x == x && food_pos.y == y)
        continue;
      if (animal_pos.x != FOOD_NOT_SET &&
          ((animal_pos.x == x && animal_pos.y == y) ||
           (animal_pos.x + 1 == x && animal_pos.y == y)))
        /*  animal is two tiles wide */
        continue;
      if (!checkPointForCollision(snake, PLAYFIELD_TO_GLOBAL_X_POS(x),
                                  PLAYFIELD_TO_GLOBAL_Y_POS(y))) {
        pos_t result = {.x = x, .y = y};
        return result;
      }
    }
  }

  // No valid position found
  pos_t result = {.x = NO_FREE_FIELDS_AVAILABLE, .y = NO_FREE_FIELDS_AVAILABLE};
  return result;
}

/**
 * Because sprintf does not support padding with %0xd in GBDK,
 * we have to do the zero padding ourselves
 *
 * @param buffer String buffer where the result will be stored
 * @param number Pointer to the number to be converted
 * @param width How long the zero padded number should be
 */
static void int_to_str_padded(char *buffer, uint16_t *number, uint8_t width) {
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

static void uint8_to_str_padded(char *buffer, uint8_t number, uint8_t width) {
  uint16_t temp = number;
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

  sprintf(buffer + i, "%d", number);
}

inline void render_steps(void) {
  char str_steps[3];
  uint8_to_str_padded(str_steps, step_counter, 2);
  vwf_draw_text(STEP_POS_X, STEP_POS_Y, 80, (unsigned char *)str_steps);
}

static void render_score(uint16_t *score) {
  char str_score[5];
  int_to_str_padded(str_score, score, 4);
  vwf_draw_text(1, 1, 60, (unsigned char *)str_score);
}

static uint8_t is_button_pressed_debounced(uint8_t mask) {
  return (joypadCurrent & mask) && !(joypadPrevious & mask);
}

static void wait_until_pressed_debounced(uint8_t mask) {
  joypadPrevious = joypadCurrent;
  joypadCurrent = joypad();
  while (!(joypadCurrent & mask) || (joypadPrevious & mask)) {
    vsync();
    joypadPrevious = joypadCurrent;
    joypadCurrent = joypad();
  }
}

static void clear_animal_related_stuff(void) {

  set_bkg_tile_xy(STEP_POS_X, STEP_POS_Y, BACKGROUND_EMPTY_TILE);
  set_bkg_tile_xy(STEP_POS_X + 1, STEP_POS_Y, BACKGROUND_EMPTY_TILE);

  switch (shown_animal_type) {
  case 0:
    move_metasprite_ex(animal_spider_metasprite, SPIDER_SPRITE, 0,
                       SPIDER_SPRITE, FOOD_NOT_SET, FOOD_NOT_SET);
    move_metasprite_ex(animal_spider_metasprite, SPIDER_SPRITE, 0, 13,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    break;
  case 1:
    move_metasprite_ex(animal_mouse_metasprite, MOUSE_SPRITE, 0, MOUSE_SPRITE,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    move_metasprite_ex(animal_mouse_metasprite, MOUSE_SPRITE, 0, 15,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    break;
  case 2:
    move_metasprite_ex(animal_fish_metasprite, FISH_SPRITE, 0, FISH_SPRITE,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    move_metasprite_ex(animal_fish_metasprite, FISH_SPRITE, 0, 17, FOOD_NOT_SET,
                       FOOD_NOT_SET);
    break;
  case 3:
    move_metasprite_ex(animal_bug_metasprite, BUG_SPRITE, 0, BUG_SPRITE,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    move_metasprite_ex(animal_bug_metasprite, BUG_SPRITE, 0, 19, FOOD_NOT_SET,
                       FOOD_NOT_SET);
    break;
  case 4:
    move_metasprite_ex(animal_turtle_metasprite, TURTLE_SPRITE, 0,
                       TURTLE_SPRITE, FOOD_NOT_SET, FOOD_NOT_SET);
    move_metasprite_ex(animal_turtle_metasprite, TURTLE_SPRITE, 0, 21,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    break;
  case 5:
    move_metasprite_ex(animal_ant_metasprite, ANT_SPRITE, 0, ANT_SPRITE,
                       FOOD_NOT_SET, FOOD_NOT_SET);
    move_metasprite_ex(animal_ant_metasprite, ANT_SPRITE, 0, 21, FOOD_NOT_SET,
                       FOOD_NOT_SET);
    break;
  }
}

static void render_level_bars(uint8_t velocity){
    uint8_t i, j;
    const uint8_t level_bar_bottom = 15;
    for (i = 2; i <= MAX_LEVEL; i++) {
      if (i <= velocity) {
        for (j = 0; j <= i; j++) {
            set_bkg_tile_xy(((i * 2) - 1), level_bar_bottom - j, FULL_BAR_TILE_MAIN);
        }
      } else {
        for (j = 0; j <= i + 1; j++) {
          set_bkg_tile_xy(((i * 2) - 1), level_bar_bottom - j, EMPTY_TILE_MAIN);
        }
      }
    }
}

void main(void) {

  direction_type snake_direction;
  direction_type dpad_direction;

  uint8_t food_counter;
  uint16_t score;
  uint16_t top_score = 0;
  uint8_t velocity = 1;
  uint8_t has_food_in_mouth;
  uint8_t has_animal_in_mouth;
  /* Bring up an animal after 5 meals (animals don't count here) */

  /*  Apply default pallete 3-2-1-0 */
  DISPLAY_ON;
  SHOW_BKG;
  SHOW_SPRITES;
  SPRITES_8x8;
  /*  Load fonts */

  vwf_load_font(0, vwf_font, BANK(vwf_font));
  vwf_set_destination(VWF_RENDER_BKG);

LevelSelection:
  set_bkg_data(snake_main_site_TILE_ORIGIN, snake_main_site_TILE_COUNT,
               snake_main_site_tiles);

  /* Alter background palette because GIMP & PNG@ASSET does not give me the
   * correct palette */
  BGP_REG = 0b11000000;

  set_bkg_tiles(0, 0, snake_main_site_WIDTH / snake_main_site_TILE_W,
                snake_main_site_HEIGHT / snake_main_site_TILE_H,
                snake_main_site_map);

  if (HasExistingSave()) {
    LoadSavedData(&top_score, &velocity);

    /*  Sanitize velocity, just in case it got corrupted */
    velocity = velocity > MAX_LEVEL ? MAX_LEVEL : velocity;
    velocity = velocity < MIN_LEVEL ? MIN_LEVEL : velocity;
  }

  vwf_draw_text(2, 5, snake_main_site_TILE_COUNT + 1,
                (unsigned char *)"LEVEL:");
  vwf_draw_text(10 - 4, 0, snake_main_site_TILE_COUNT + 1 + 7,
                (unsigned char *)"TOP SCORE");
  char str_buffer[20];
  int_to_str_padded(str_buffer, &top_score, 4);
  vwf_draw_text(10 - 1, 1, snake_main_site_TILE_COUNT + 1 + 7 + 10,
                (unsigned char *)str_buffer);

  render_level_bars(velocity);
  joypadPrevious = joypadCurrent;
  joypadCurrent = joypad();
  uint8_t start_game_mask = (J_START | J_A);
  uint8_t incr_level_mask = (J_UP | J_RIGHT);
  uint8_t decr_level_mask = (J_DOWN | J_LEFT);

  while (!(joypadCurrent & start_game_mask)) {
    if ((joypadCurrent & incr_level_mask) &&
        !(joypadPrevious & incr_level_mask)) {
      velocity = ++velocity > MAX_LEVEL ? MAX_LEVEL : velocity;
    } else if ((joypadCurrent & decr_level_mask) &&
               !(joypadPrevious & decr_level_mask)) {
      velocity = --velocity < MIN_LEVEL ? MIN_LEVEL : velocity;
    }

    render_level_bars(velocity);

    /*  get new input */
    vsync();
    joypadPrevious = joypadCurrent;
    joypadCurrent = joypad();
  }

  /*  Button to start game pressed */

  SaveData(&top_score,  &velocity);

  //  wait_until_pressed_debounced(J_START | J_A | J_B);

  /* Restore default palette for background */
  BGP_REG = 0b11100100;
  initrand(DIV_REG); /* Seed with the Game Boy's divider register */

  /*  reload background tiles for main game */
  set_bkg_data(0, 39 , snake_bckg_tileset);

  set_sprite_data(FOOD_SPRITE, 1, food);
  set_sprite_data(SPIDER_SPRITE, 2, &animals_tiles[0]);
  set_sprite_data(MOUSE_SPRITE, 2, &animals_tiles[ANIMAL_MOUSE_1 * TILE_SIZE]);
  set_sprite_data(FISH_SPRITE, 2, &animals_tiles[ANIMAL_FISH_1 * TILE_SIZE]);
  set_sprite_data(BUG_SPRITE, 2, &animals_tiles[ANIMAL_BUG_1 * TILE_SIZE]);
  set_sprite_data(TURTLE_SPRITE, 2,
                  &animals_tiles[ANIMAL_TURTLE_1 * TILE_SIZE]);
  set_sprite_data(ANT_SPRITE, 2, &animals_tiles[ANIMAL_ANT_1 * TILE_SIZE]);

GameStart:

  has_animal_in_mouth = 0;
  has_food_in_mouth = 0;
  uint8_t current_food_threshold =
      distribution_required_food_for_animals[rand() % 10];
  animal_pos.x = FOOD_NOT_SET;
  animal_pos.y = FOOD_NOT_SET;
  clear_animal_related_stuff();
  food_counter = 0;

  /*  main game background with borders */
  set_bkg_tiles(0, 0, 20, 18, snake_bckg);

  score = 0;
  render_score(&score);

  snake_direction = START_DIRECTION;
  dpad_direction = START_DIRECTION;

  uint16_t i;
  for (i = 0; i < MAX_NODES; i++) {
    freeNode(&node_pool[i]);
  }

  snake_t snake;
  init_default_snake(&snake);

  /*  render the snake for the first time */

  snake_node_t *node_to_render = snake.head;
  while (node_to_render != NULL) {
    set_bkg_tile_xy(node_to_render->x_pos, node_to_render->y_pos,
                    node_to_render->tile_to_render);
    node_to_render = node_to_render->next_node;
  }

  /*  randomly place food on a free spot */

  food_pos = get_random_free_food_position(&snake, 0);
  if (food_pos.x == NO_FREE_FIELDS_AVAILABLE){
    goto GameOver;
  }
  move_sprite(FOOD_SPRITE, (uint8_t)PLAYFIELD_TO_SPRITE_X_POS(food_pos.x),
              (uint8_t)PLAYFIELD_TO_SPRITE_Y_POS(food_pos.y));

  /*  Flag to check if a movement is pending to get rendered before catching a
   * new one */

  uint8_t movement_pending = 0;

  /*  Gets called at 60fps  */

  uint16_t update_screen_counter = 0;

  while (1) {

    joypadPrevious = joypadCurrent;
    joypadCurrent = joypad();

    if ((joypadCurrent & J_START) && !(joypadPrevious & J_START)) {

      /*  Take a snapshot of the current background */
      uint8_t background_data_snake[20 * 18];
      wait_vbl_done(); // Wait for VBlank before reading
      get_bkg_tiles(0, 0, 20, 18, &background_data_snake[0]);

      /*  Just show borders without snake */
      set_bkg_tiles(0, 0, 20, 18, snake_bckg);
      vwf_draw_text(STARTPOS_X - 2, STARTPOS_Y, 70, "Paused");
      render_score(&score);
      vsync();

      wait_until_pressed_debounced(J_START);
      /*  restore snake and give a small delay to the user until game continues */
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

    if (update_screen_counter % (velocity_rates_counter_values[velocity - 1]) == 0) {

      if (animal_pos.x != FOOD_NOT_SET) {
        /*  Animal is currently shown on screen */
        if (step_counter > 1) {
          step_counter--;
          render_steps();
        } else {

          /* reset position and remove animal sprites */
          animal_pos.x = FOOD_NOT_SET;
          animal_pos.y = FOOD_NOT_SET;

          /*  clear step score */
          clear_animal_related_stuff();
        }
      }

      /*  
       *  Instead of moving and updating every node in the linked list when
       *  the snake moves forward, we just insert a new node after the head
       *  and remove the node before the tail.
       */

      snake_node_t *new_node = allocateNode();
      if (new_node) {
        new_node->x_pos = snake.head->x_pos;
        new_node->y_pos = snake.head->y_pos;
        new_node->dir_to_next_node = snake.head->dir_to_next_node;
        new_node->tile_to_render = BACKGROUND_EMPTY_TILE;
        new_node->next_node = snake.head->next_node;
        new_node->prev_node = snake.head;
      }

      /* 
       * Find which snake tile must get rendered for the new inserted node
       * Direction no next node is opposite to current moving direction
       */

      direction_type dir_n = snake.head->dir_to_next_node;


      direction_type opposite_dir = OPPOSITE_DIRECTION(snake_direction);
      snake.head->dir_to_next_node = opposite_dir;
      if (has_food_in_mouth || has_animal_in_mouth) {
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

      else {
        /* normal body tile without food eaten */
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

      /* Inject new node */

      snake.head->next_node->prev_node = new_node;
      snake.head->next_node = new_node;

      /* Update coordinates for the head */

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
        anticipated_next_pos.y = PLAYFIELD_BOTTOM;
      } else if (anticipated_next_pos.y > PLAYFIELD_BOTTOM) {
        anticipated_next_pos.y = PLAYFIELD_Y_OFFSET;
      }

      /*  Check if snake will bite itself */

      if (checkPointForCollision(&snake, anticipated_next_pos.x,
                                 anticipated_next_pos.y)) {

        /*  Edge case: if the snake will bite in the tail, the game will
         * continue */

        if (!((snake.tail->x_pos == anticipated_next_pos.x) &&
              (snake.tail->y_pos == anticipated_next_pos.y))) {

GameOver: 
          clear_animal_related_stuff();

          if (score > top_score) {
            top_score = score;
            SaveData(&top_score, &velocity);
            vwf_draw_text(6, 1, 90, "NEW TOP SCORE");
          }

          uint8_t background_data_snake[20 * 18];
          wait_vbl_done(); // Wait for VBlank before reading
          get_bkg_tiles(0, 0, 20, 18, &background_data_snake[0]);

          play_game_over_sound();
          uint8_t empty_playfield_bg[PLAYFIELD_SIZE];
          uint16_t i;
          for (i = 0; i < PLAYFIELD_SIZE; i++) {
            empty_playfield_bg[i] = BACKGROUND_EMPTY_TILE;
          }

          for (i = 0; i < 4; i++) {
            /*  Clear snake from screen, just show borders */
            set_bkg_tiles(1, PLAYFIELD_Y_OFFSET, PLAYFIELD_X_MAX + 1,
                          PLAYFIELD_Y_MAX + 1, empty_playfield_bg);
            delay(FLASH_INTERVAL);

            /* show snake on background */
            set_bkg_tiles(0, 0, 20, 18, background_data_snake);
            delay(FLASH_INTERVAL);
          }

          wait_until_pressed_debounced(J_START | J_A | J_B);
          if (joypadCurrent & (J_START | J_A)) {
            goto GameStart;
          } else {
            move_sprite(FOOD_SPRITE, FOOD_NOT_SET, FOOD_NOT_SET);
            goto LevelSelection;
          }
        }
      }

      snake.head->y_pos = anticipated_next_pos.y;
      snake.head->x_pos = anticipated_next_pos.x;

      /*  Update tail and remove node before tail */

      if (!has_food_in_mouth) {
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

      if (checkPointForCollision(&snake, PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x),
                                 PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y))) {
        has_food_in_mouth = 1;
        score += velocity;
        render_score(&score);

        food_counter++;
        if (food_counter >= current_food_threshold) {

          /*  Time to show an animal */
          step_counter = 20;

          current_food_threshold =
              distribution_required_food_for_animals[rand() % 10];
          food_counter = 0;

          /*  Draw a random position to place the animal */

          animal_pos = get_random_free_food_position(&snake, 1);
          if (animal_pos.x == NO_FREE_FIELDS_AVAILABLE){
            goto GameOver;
          }

          shown_animal_type = rand() % NB_OF_DIFFERENT_ANIMALS;
          uint8_t sprite_x = PLAYFIELD_TO_SPRITE_X_POS(animal_pos.x);
          uint8_t sprite_y = PLAYFIELD_TO_SPRITE_Y_POS(animal_pos.y);

          switch (shown_animal_type) {
          case 0:
            move_metasprite_ex(animal_spider_metasprite, SPIDER_SPRITE, 0,
                               SPIDER_SPRITE, sprite_x, sprite_y);
            move_metasprite_ex(animal_spider_metasprite, SPIDER_SPRITE, 0, 13,
                               ANIMAL_TOPBAR_X_POS, ANIMAL_TOPBAR_Y_POS);
            break;
          case 1:
            move_metasprite_ex(animal_mouse_metasprite, MOUSE_SPRITE, 0,
                               MOUSE_SPRITE, sprite_x, sprite_y);
            move_metasprite_ex(animal_mouse_metasprite, MOUSE_SPRITE, 0, 15,
                               ANIMAL_TOPBAR_X_POS, ANIMAL_TOPBAR_Y_POS);
            break;
          case 2:
            move_metasprite_ex(animal_fish_metasprite, FISH_SPRITE, 0,
                               FISH_SPRITE, sprite_x, sprite_y);
            move_metasprite_ex(animal_fish_metasprite, FISH_SPRITE, 0, 17,
                               ANIMAL_TOPBAR_X_POS, ANIMAL_TOPBAR_Y_POS);
            break;
          case 3:
            move_metasprite_ex(animal_bug_metasprite, BUG_SPRITE, 0, BUG_SPRITE,
                               sprite_x, sprite_y);
            move_metasprite_ex(animal_bug_metasprite, BUG_SPRITE, 0, 19,
                               ANIMAL_TOPBAR_X_POS, ANIMAL_TOPBAR_Y_POS);
            break;
          case 4:
            move_metasprite_ex(animal_turtle_metasprite, TURTLE_SPRITE, 0,
                               TURTLE_SPRITE, sprite_x, sprite_y);
            move_metasprite_ex(animal_turtle_metasprite, TURTLE_SPRITE, 0, 21,
                               ANIMAL_TOPBAR_X_POS, ANIMAL_TOPBAR_Y_POS);
            break;
          case 5:
            move_metasprite_ex(animal_ant_metasprite, ANT_SPRITE, 0, ANT_SPRITE,
                               sprite_x, sprite_y);
            move_metasprite_ex(animal_ant_metasprite, ANT_SPRITE, 0, 21,
                               ANIMAL_TOPBAR_X_POS, ANIMAL_TOPBAR_Y_POS);
            break;
          }
        }

        food_pos = get_random_free_food_position(&snake, 0);

        if (food_pos.x == NO_FREE_FIELDS_AVAILABLE) {
          goto GameOver;
        }

        move_sprite(FOOD_SPRITE, (uint8_t)PLAYFIELD_TO_SPRITE_X_POS(food_pos.x),
                    (uint8_t)PLAYFIELD_TO_SPRITE_Y_POS(food_pos.y));

        play_eating_sound();
      } else if (checkPointForCollision(
                     &snake, PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x),
                     PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y)) ||
                 checkPointForCollision(
                     &snake, PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x + 1),
                     PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y))) {

        animal_pos.x = FOOD_NOT_SET;
        animal_pos.y = FOOD_NOT_SET;
        clear_animal_related_stuff();
        score += (45 + (velocity * 5)) - ((20 - step_counter) * 2);
        render_score(&score);

        play_eating_sound();
        /*  a different flag to show the correct tile, but when eating an
         * animal, the snake should not grow */
        has_animal_in_mouth = 1;
      } else {
        has_food_in_mouth = 0;
        has_animal_in_mouth = 0;
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
        if (snake.head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)) {
          break;
        };
        if (snake.head->y_pos - 1 == PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y) == PLAYFIELD_BOTTOM) {
          if (snake.head->y_pos - 1 < PLAYFIELD_Y_OFFSET) {
            food_lies_ahead = 1;
          }
        }
        break;
      case RIGHT:
        if (snake.head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)) {
          break;
        };
        if (snake.head->x_pos + 1 == PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x) == 1) {
          if (snake.head->x_pos + 1 > PLAYFIELD_WIDTH) {
            food_lies_ahead = 1;
          }
        }
        break;
      case DOWN:
        if (snake.head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)) {
          break;
        };
        if (snake.head->y_pos + 1 == PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y) == PLAYFIELD_Y_OFFSET) {
          if (snake.head->y_pos + 1 > PLAYFIELD_BOTTOM) {
            food_lies_ahead = 1;
          }
        }
        break;
      case LEFT:
        if (snake.head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(food_pos.y)) {
          break;
        };
        if (snake.head->x_pos - 1 == PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_X_POS(food_pos.x) == PLAYFIELD_WIDTH) {
          if (snake.head->x_pos - 1 < 1) {
            food_lies_ahead = 1;
          }
        }
      }

      /* Now check again for animals */

      switch (snake_direction) {
      case UP:
        if (snake.head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x) &&
            snake.head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x + 1)) {
          break;
        };
        if (snake.head->y_pos - 1 == PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y) == PLAYFIELD_BOTTOM) {
          if (snake.head->y_pos - 1 < PLAYFIELD_Y_OFFSET) {
            food_lies_ahead = 1;
          }
        }
        break;
      case RIGHT:
        if (snake.head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y)) {
          break;
        };
        if (snake.head->x_pos + 1 == PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x) == 1) {
          if (snake.head->x_pos + 1 > PLAYFIELD_WIDTH) {
            food_lies_ahead = 1;
          }
        }
        break;
      case DOWN:
        if (snake.head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x) &&
            snake.head->x_pos != PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x + 1)) {
          break;
        };
        if (snake.head->y_pos + 1 == PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y) ==
                 PLAYFIELD_Y_OFFSET) {
          if (snake.head->y_pos + 1 > PLAYFIELD_BOTTOM) {
            food_lies_ahead = 1;
          }
        }
        break;
      case LEFT:
        if (snake.head->y_pos != PLAYFIELD_TO_GLOBAL_Y_POS(animal_pos.y)) {
          break;
        };
        if (snake.head->x_pos - 1 ==
            PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x + 1)) {
          food_lies_ahead = 1;
        }
        /* Check wrapped around position */
        else if (PLAYFIELD_TO_GLOBAL_X_POS(animal_pos.x + 1) ==
                 PLAYFIELD_WIDTH) {
          if (snake.head->x_pos - 1 < 1) {
            food_lies_ahead = 1;
          }
        }
      }

      /*  Update background tiles */

      snake_node_t *node_after_head = snake.head->next_node;
      set_bkg_tile_xy(node_after_head->x_pos, node_after_head->y_pos,
                      node_after_head->tile_to_render);
      set_bkg_tile_xy(snake.tail->x_pos, snake.tail->y_pos,
                      snake.tail->tile_to_render);

      if (food_lies_ahead) {
        set_bkg_tile_xy(snake.head->x_pos, snake.head->y_pos,
                        snake_direction + SNAKE_MOUTH_OPEN_UP);
      } else {
        set_bkg_tile_xy(snake.head->x_pos, snake.head->y_pos, snake_direction);
      }

    }
    update_screen_counter++;

    /* Done processing, yield CPU and wait for start of next frame */
    vsync();
  }
}
