#include "main.h"
#include "snake.h"
#include <assert.h>
#include <stdint.h>


/**
 * Since the GB has very limited RAM, using heap will lead to fragmented
 * memory. Thus, we use a memory pool for nodes
 */
snake_node_t node_pool[MAX_NODES];

/**
 * Allocate a node from the pool (aka malloc())
 *
 * @return Pointer to the availble node. NULL if no node is availble
 */
snake_node_t *allocateNode(void) {
  uint16_t i;
  for (i = 0; i < MAX_NODES; i++) {
    if (!node_pool[i].active) {
      node_pool[i].active = 1;
      return &node_pool[i];
    }
  }
  return NULL;
}

/**
 * Frees up a node fom the pool (aka free())
 *
 * @param obj The object to free up
 */
void freeNode(snake_node_t *obj) { obj->active = 0; }

/**
 * Check if a point (snake head or food) collides with the snake
 *
 * @param snake: the linked list with all snake nodes.
 * @param x: x position in global coordination (map) system.
 * @param y: y position in global coordination (map) system.
 *
 * @return 1 if there was a collision, otherwise 0.
 */
uint8_t checkPointForCollision(snake_t *snake, uint8_t x, uint8_t y) {

  snake_node_t *node = snake->head;
  do {
    if (node->x_pos == x && node->y_pos == y) {
      return 1;
    }
    node = node->next_node;
  } while (node != NULL);

  return 0;
}

void init_default_snake(snake_t *snake) {

/**
 *  Define how the default snake should look like when starting a new game.
 *  By definition, the first entry is the head, the last entry the tail
 */
#ifndef DEBUG_SNAKE
  const snake_node_t default_snake[] = {
      {.x_pos = STARTPOS_X,
       .y_pos = STARTPOS_Y,
       .tile_to_render = SNAKE_HEAD_RIGHT_TILE,
       .dir_to_next_node = LEFT},
      {.x_pos = STARTPOS_X - 1,
       .y_pos = STARTPOS_Y,
       .tile_to_render = SNAKE_BODY_STRAIGHT_RIGHT,
       .dir_to_next_node = LEFT},
      {.x_pos = STARTPOS_X - 2,
       .y_pos = STARTPOS_Y,
       .tile_to_render = SNAKE_BODY_STRAIGHT_RIGHT,
       .dir_to_next_node = LEFT},
      {
          .x_pos = STARTPOS_X - 3,
          .y_pos = STARTPOS_Y,
          .tile_to_render = SNAKE_TAIL_RIGHT,
      },
  };
#endif
#ifdef DEBUG_SNAKE
  /*  Here we create a dummy snake that takes almost all available
   *  fields.
   *  */
  const uint8_t w = PLAYFIELD_WIDTH;
  const uint8_t h = 12;

  snake_node_t default_snake[PLAYFIELD_WIDTH * 12];
  snake_node_t *default_snake_pointer = default_snake;
  uint8_t x, y;
  for (y = 0; y < h; y++) {
    if (y % 2 == 0) {
      /*  build snake body from right to left */
      for (x = w; x > 0; x--) {
        snake_node_t new_node = {
            .x_pos = PLAYFIELD_TO_GLOBAL_X_POS(x - 1),
            .y_pos = PLAYFIELD_TO_GLOBAL_Y_POS(y),
            .tile_to_render = SNAKE_BODY_STRAIGHT_LEFT,
            .dir_to_next_node = LEFT,
        };
        *default_snake_pointer = new_node;
        default_snake_pointer++;
      }
    } else {
      /*  build snake body from left to right */
      for (x = 0; x < w; x++) {
        snake_node_t new_node = {
            .x_pos = PLAYFIELD_TO_GLOBAL_X_POS(x),
            .y_pos = PLAYFIELD_TO_GLOBAL_Y_POS(y),
            .tile_to_render = SNAKE_BODY_STRAIGHT_LEFT,
            .dir_to_next_node = RIGHT,
        };
        *default_snake_pointer = new_node;
        default_snake_pointer++;
      }
    }
  }
#endif

  uint16_t len = sizeof(default_snake) / sizeof(snake_node_t);
  uint16_t i;
  const snake_node_t *current_node = default_snake;
  snake_node_t *prev_node = NULL;
  for (i = 0; i < len; i++) {
    snake_node_t *node = allocateNode();
    /*  copy properties from default snake */

    node->x_pos = current_node->x_pos;
    node->y_pos = current_node->y_pos;
    node->tile_to_render = current_node->tile_to_render;
    node->dir_to_next_node = current_node->dir_to_next_node;

    if (i == 0) {
      /*  handle first element as head */
      node->prev_node = NULL;
      snake->head = node;
    } else if (i == len - 1) {
      /*  handle last element as tail */
      node->next_node = NULL;
      node->prev_node = prev_node;
      prev_node->next_node = node;
      snake->tail = node;
    } else {
      node->prev_node = prev_node;
      prev_node->next_node = node;
    }
    prev_node = node;
    /*  Move advancing pointer */
    current_node++;
  }
}
