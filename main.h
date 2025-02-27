#ifndef _MAIN_H
#define _MAIN_H


#include <stdint.h>
#define PLAYFIELD_HEIGHT ((144 / 8) - 1 - 1)
#define PLAYFIELD_WIDTH ((160 / 8) - 1 - 1)
#define MAX_NODES (PLAYFIELD_WIDTH * PLAYFIELD_HEIGHT)

typedef enum {
  UP,
  RIGHT,
  DOWN,
  LEFT,
} direction_type;

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
};

typedef struct pos_s {
  uint8_t x;
  uint8_t y;
}pos_t;

#endif
