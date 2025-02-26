#include "snake.h"
#include "main.h"
#include <gb/gb.h>

extern snake_node_t node_pool[MAX_NODES];

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

void freeNode(snake_node_t* obj) {
    obj->active = 0;
}
