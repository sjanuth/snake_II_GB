#include "snake.h"
#include "main.h"
#include <gb/gb.h>

extern snake_node_t node_pool[MAX_NODES];

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
void freeNode(snake_node_t* obj) {
    obj->active = 0;
}

/**
 * Check if a point (snake head or food) collides with the snake
 *
 * @param snake: the linked list with all snake nodes.
 * @param x: x position in global coordination (map) system.
 * @param y: y position in global coordination (map) system.
 *
 * @return 1 if there was a collision, otherwise 0.
 */
uint8_t checkPointForCollision(snake_t *snake, uint8_t x, uint8_t y){

  snake_node_t *node = snake->head;
  do {
    if(node->x_pos == x && node->y_pos == y){return 1;}
    node = node->next_node;
  }while (node != NULL);

  return 0;
}
