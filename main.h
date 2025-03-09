#ifndef _MAIN_H
#define _MAIN_H

#include <stdint.h>
#include <gb/gb.h>

/*  2 tiles for score, 1 tile for border, 1 tile for grid */

#define PLAYFIELD_Y_OFFSET (4)
#define PLAYFIELD_BOTTOM ((144 / 8) - 1 - 1)
#define PLAYFIELD_HEIGHT ((144 / 8) - PLAYFIELD_Y_OFFSET- 1)
#define PLAYFIELD_WIDTH ((160 / 8) - 1 - 1)
#define MAX_NODES (PLAYFIELD_WIDTH * PLAYFIELD_HEIGHT)

typedef enum {
  UP,
  RIGHT,
  DOWN,
  LEFT,
} direction_type;

/*  animal sprites use two tiles to build a metasprite */
enum {
  FOOD_SPRITE,
  SPIDER_SPRITE,
  MOUSE_SPRITE = 3,
  FISH_SPRITE = 5,
  BUG_SPRITE = 7,
  TURTLE_SPRITE = 9,
  ANT_SPRITE = 11,
};

enum {
  SNAKE_HEAD_UP_TILE,
  SNAKE_HEAD_RIGHT_TILE,
  SNAKE_HEAD_DOWN_TILE,
  SNAKE_HEAD_LEFT_TILE
};

enum {
  SNAKE_BODY_STRAIGHT_UP= 10,
  SNAKE_BODY_STRAIGHT_RIGHT,
  SNAKE_BODY_STRAIGHT_DOWN,
  SNAKE_BODY_STRAIGHT_LEFT,
  SNAKE_BODY_UP_RIGHT,
  SNAKE_BODY_RIGHT_DOWN,
  SNAKE_BODY_DOWN_LEFT,
  SNAKE_BODY_LEFT_UP,
  BACKGROUND_EMPTY_TILE,
  SNAKE_FOOD,
  SNAKE_TAIL_UP,
  SNAKE_TAIL_RIGHT,
  SNAKE_TAIL_DOWN,
  SNAKE_TAIL_LEFT,
  SNAKE_MOUTH_OPEN_UP,
  SNAKE_MOUTH_OPEN_RIGHT,
  SNAKE_MOUTH_OPEN_DOWN,
  SNAKE_MOUTH_OPEN_LEFT,
  SNAKE_FOOD_EATEN_UP_RIGTH,
  SNAKE_FOOD_EATEN_RIGHT_DOWN,
  SNAKE_FOOD_EATEN_DOWN_LEFT,
  SNAKE_FOOD_EATEN_LEFT_UP,
  SNAKE_FOOD_EATEN_UP,
  SNAKE_FOOD_EATEN_RIGHT,
  SNAKE_FOOD_EATEN_DOWN,
  SNAKE_FOOD_EATEN_LEFT,
};

enum{
  ANIMAL_SPIDER_1,
  ANIMAL_SPIDER_2,
  ANIMAL_MOUSE_1,
  ANIMAL_MOUSE_2,
  ANIMAL_FISH_1,
  ANIMAL_FISH_2,
  ANIMAL_BUG_1,
  ANIMAL_BUG_2,
  ANIMAL_TURTLE_1,
  ANIMAL_TURTLE_2,
  ANIMAL_ANT_1,
  ANIMAL_ANT_2,
};

typedef struct pos_s {
  uint8_t x;
  uint8_t y;
}pos_t;

#endif
