#include "coroutine.h"
#include <stdio.h>
#include <stdlib.h>

int tmp;


void producor(void *seed) {
  int i;

  for(i = 0; i < 10; ++i) {
    tmp = *((int *)seed) + i;
    coroutine_yield();
  } 
}

void consumer(void *ud) {
  for (;;) {
    printf("consume: %d\n", tmp);
    coroutine_yield();
  
  }
}

int main(int argc, const char *argv[]) {
  int seed = 100;

  int co_prod = coroutine_create(producor, (void *)&seed);
  int co_cons = coroutine_create(consumer, NULL);

  while (coroutine_status(co_prod) != CO_STATUS_DEAD
      && coroutine_status(co_cons) != CO_STATUS_DEAD) {
    coroutine_resume(co_prod);
    coroutine_resume(co_cons);
  }
  
  return 0;
}
