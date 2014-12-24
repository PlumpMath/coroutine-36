#include "coroutine.h"
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#define DEFAULT_STACK_SIZE      (4 * 1024 * 1024)
#define DEFAULT_COROUTINE_NUM   16
#define EXTRA_STACK             64

struct coroutine;
struct schedule;

static struct schedule *g_sched = NULL;

struct schedule {
  jmp_buf ctx;
  int cap;
  int nco;
  int running;
  struct coroutine *cos[];
};

struct stack {
  int size;
  char *sp;
  char base[];
};

struct coroutine {
  jmp_buf ctx;
  int status;
  coroutine_func func;
  void *ud;
  struct stack *stk;
};

static struct stack * alloc_stack(int size) {
  assert(size > 0);

  int allocsize = sizeof (struct stack) + (size + EXTRA_STACK) * sizeof(char);
  struct stack *stk = malloc(allocsize);
  stk->size = allocsize;
  stk->sp = stk->base + size;
  if ((unsigned long)stk->sp & 0x3f) {
    stk->sp = stk->sp - ((unsigned long)stk->sp & 0x3f);
  }

  return stk;
}

static void free_stack(struct stack *stk) {
  free(stk);
}

static struct coroutine * alloc_coroutine(coroutine_func func, void *ud) {
  struct coroutine *co = malloc(sizeof (*co));
  co->func = func;
  co->ud = ud;
  co->stk = alloc_stack(DEFAULT_STACK_SIZE);
  return co;
}

static void free_coroutine(struct coroutine *co) {
  free_stack(co->stk);
  free(co);
} 

static struct schedule * alloc_schedule() {
  struct schedule *sched = malloc(sizeof (struct schedule) + DEFAULT_COROUTINE_NUM * sizeof (struct coroutine *));
  memset(sched->cos, 0, sizeof (struct coroutine*) * DEFAULT_COROUTINE_NUM);
  sched->cap = DEFAULT_COROUTINE_NUM;
  sched->nco = 1;
  sched->running = 0;
  return sched;
}

static void free_schedule(struct schedule *sched) {
  int i;
  for (i = 0; i < sched->cap; ++i) {
    if (sched->cos[i] != NULL) {
      free_coroutine(sched->cos[i]);
    }
  }

  free(sched);
}

static void main_func() {
  char a;
  int id = g_sched->running;
  assert((0 < id) && (id < g_sched->cap));
  struct coroutine *co = g_sched->cos[id];
  assert(&a < co->stk->sp);

  co->func(co->ud);

  co->status = CO_STATUS_DEAD;
  longjmp(g_sched->ctx, 1);

  assert(0); 
}

static int expand_schedule() {
  int newsize = sizeof (*g_sched) + (g_sched->cap * 2) * sizeof (struct coroutine *);
  struct schedule *newsched = realloc(g_sched, newsize);

  memset(newsched->cos + newsched->cap, 0, sizeof (struct coroutine *) * newsched->cap);
  newsched->cap *= 2;
  g_sched = newsched;
  return 0;
}

int coroutine_create(coroutine_func func, void *ud) {
  if (g_sched == NULL) {
    g_sched = alloc_schedule(); 
  } 

  if (g_sched->nco >= g_sched->cap) {
    expand_schedule();
  }


  struct coroutine *co = alloc_coroutine(func, ud);

  char *orsp;
  asm volatile (
      "mov  %%rsp, %0          \n\t"
      "mov  %1, %%rsp          \n\t"
      : "=m"(orsp)
      : "m"(co->stk->sp));

  if (setjmp(co->ctx)) {
    main_func(); 
  }

  asm volatile (
      "mov  %0, %%rsp           \n\t"
      : 
      : "m"(orsp)
      );

  co->status = CO_STATUS_READY;

  int id = g_sched->nco;
  int i;
  for (i = 0; i < g_sched->cap; ++i, id++) {
    if (id >= g_sched->cap) {
      id = 1; 
    }
    if (g_sched->cos[id]  == NULL) {
      g_sched->cos[id] = co;
      g_sched->nco++;
      return id;
    }
  }

  assert(0);
  return -1;
}

int coroutine_resume(int id) {
  assert(g_sched->running == 0);
  if (id <= 0 || id >= g_sched->cap) {
    return -1; 
  }

  struct coroutine *co = g_sched->cos[id];
  if (co == NULL) {
    return -1; 
  }
  
  if (co->status == CO_STATUS_READY || co->status == CO_STATUS_SUSPEND) {
    if (setjmp(g_sched->ctx) == 0) {
      g_sched->running = id;
      co->status = CO_STATUS_RUNNING;
      longjmp(co->ctx, 1);
    }

    co = g_sched->cos[g_sched->running];
    if (co->status == CO_STATUS_DEAD) {
      assert(g_sched->nco > 1);
      g_sched->nco--;
      g_sched->cos[g_sched->running] = NULL;
      free_coroutine(co);
    }

    g_sched->running = 0;

    if (g_sched->nco == 1) {
      free_schedule(g_sched);
      g_sched = NULL;
    }
    return 0;
  }

  assert(0);
  return -1;
} 

void coroutine_yield() {
  assert(g_sched->running != 0);
  struct coroutine *co = g_sched->cos[g_sched->running];

  if (setjmp(co->ctx) == 0) {
    co->status = CO_STATUS_SUSPEND;
    longjmp(g_sched->ctx, 1); 
  }
}

int coroutine_running() {
  return g_sched->running;
}

int coroutine_status(int id) {
  if (g_sched == NULL) {
    return CO_STATUS_DEAD;
  }

  if (id <= 0 || id >= g_sched->cap) {
    return CO_STATUS_DEAD;
  }
  
  if (g_sched->cos[id] == NULL) {
    return CO_STATUS_DEAD; 
  }

  return g_sched->cos[id]->status;
}
