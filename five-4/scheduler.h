#include <stdio.h>
#include <stdlib.h>
#include <atomic_ops.h>
#include "queue.h"

#define STACK_SIZE 4*1024*1024
#define current_thread (get_current_thread())

extern void * safe_mem(int, void*);
#define malloc(arg) safe_mem(0, ((void*)(arg)))
#define free(arg) safe_mem(1, arg)

typedef enum {
	RUNNING,
	READY, 
	BLOCKED, 
	DONE
} state_t;

struct mutex {
	int held;
        struct queue waiting_threads;
	AO_TS_t waiting_threads_lock;
} typedef mutex;

struct condition {
	struct queue waiting_threads;
	AO_TS_t waiting_condition_lock;
} typedef condition;

struct thread {
        unsigned char* stack_pointer;
        void (*initial_function)(void*);
        void* initial_argument;
	state_t state;
	mutex mutex;
	condition condition;
} typedef thread;

void set_current_thread(thread *);
thread *get_current_thread(void);

int kernel_thread_begin();

/* Both thread_start and thread_switch must have the ready
 * list lock acquired and all threads out must have the lock released
 */
void thread_start(thread *old, thread *new);
void thread_switch(thread * old, thread *new);

void mutex_init(mutex *);
void mutex_lock(mutex *);
void mutex_unlock(mutex *);

void condition_init(condition *c);
void condition_wait(condition *c, mutex *m);
void condition_signal(condition *c);
void condition_broadcast(condition *c);

void spinlock_lock(AO_TS_t *);
void spinlock_unlock(AO_TS_t *);
void block(AO_TS_t *);

void thread_join(thread*);

void scheduler_begin(int threads);
thread * thread_fork(void(*target)(void*), void *arg);
void yield();
void scheduler_end();

