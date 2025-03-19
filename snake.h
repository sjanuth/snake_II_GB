#ifndef _snake_h
#define _snake_h

#include <stdint.h>
#include "main.h"


typedef struct snake_node_s {
  uint8_t tile_to_render;
  uint8_t x_pos;
  uint8_t y_pos;
  struct snake_node_s *next_node;
  struct snake_node_s *prev_node;
  direction_type dir_to_next_node;
  uint8_t active;
} snake_node_t;

typedef struct snake_s {
  snake_node_t *head;
  snake_node_t *tail;
} snake_t;

snake_node_t *allocateNode(void) ;

void freeNode(snake_node_t* obj) ;

uint8_t checkPointForCollision(snake_t *snake, uint8_t x, uint8_t y);


#endif
