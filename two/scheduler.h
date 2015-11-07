#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE 1024*1024

typedef enum {
	RUNNING,
	READY, 
	BLOCKED, 
	DONE
} state_t;

struct thread
{
        unsigned char* stack_pointer;
        void (*initial_function)(void*);
        void* initial_argument;
	state_t state;
} typedef thread;

extern thread *current_thread;

void thread_start(thread *old, thread *new);
void thread_switch(thread * old, thread *new);

void scheduler_begin();
void thread_fork(void(*target)(void*), void *arg);
void yield();
void scheduler_end();

