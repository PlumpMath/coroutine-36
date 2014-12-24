#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#define CO_STATUS_READY     0
#define CO_STATUS_RUNNING   1
#define CO_STATUS_SUSPEND   2
#define CO_STATUS_DEAD      3

typedef void (*coroutine_func)(void *ud);

int coroutine_create(coroutine_func func, void *ud);
int coroutine_resume(int id);
int coroutine_status(int id);
int coroutine_running();
void coroutine_yield();

#endif /* end of include guard: __COROUTINE_H__ */
