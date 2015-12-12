#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>

#include "scheduler.h"

struct queue ready_list;
AO_TS_t ready_list_lock = AO_TS_INITIALIZER;

#undef malloc
#undef free
void * safe_mem(int op, void * arg) {
	static AO_TS_t spinlock = AO_TS_INITIALIZER;
	void * result = 0;

	spinlock_lock(&spinlock);
	if(op == 0) {
		result = malloc((size_t)arg);
	} else {
		free(arg);
	}
	spinlock_unlock(&spinlock);
	return result;
}
#define malloc(arg) safe_mem(0, ((void*)(arg)))
#define free(arg) safe_mem(1, arg)

// Initialize mutex 0 == unlocked 1 == locked
void mutex_init(mutex *m)
{
        m->held = 0;
        m->waiting_threads.head = NULL;
        m->waiting_threads.tail = NULL;
	m->waiting_threads_lock = AO_TS_INITIALIZER;
}

void mutex_lock(mutex *m)
{
	spinlock_lock(&m->waiting_threads_lock);	
        if (m->held == 0) {
                m->held = 1;
		spinlock_unlock(&m->waiting_threads_lock);
        } else {
                // Lock is held, block current thread
                current_thread->state = BLOCKED;
                thread_enqueue(&m->waiting_threads, current_thread);
		block(&m->waiting_threads_lock);
        }
}

void mutex_unlock(mutex *m)
{
	spinlock_lock(&m->waiting_threads_lock);
	if (!is_empty(&m->waiting_threads)) {
		m->held = 1;
		thread *temp = thread_dequeue(&m->waiting_threads);
		spinlock_lock(&ready_list_lock);
		temp->state = READY;
		thread_enqueue(&ready_list, temp);
		spinlock_unlock(&ready_list_lock);
	} else {
		m->held = 0;	
	}
	spinlock_unlock(&m->waiting_threads_lock);
}

void condition_init(condition *c) 
{
	c->waiting_threads.head = NULL;
	c->waiting_threads.tail = NULL;
	c->waiting_condition_lock = AO_TS_INITIALIZER;
}

void condition_wait(condition *c, mutex *m) 
{
	mutex_unlock(m);
	spinlock_lock(&c->waiting_condition_lock);
	current_thread->state = BLOCKED;
	thread_enqueue(&c->waiting_threads, current_thread);
	block(&c->waiting_condition_lock);
	mutex_lock(m);		
}
	
void condition_signal(condition *c)
{
	spinlock_lock(&c->waiting_condition_lock);
	if (!is_empty(&c->waiting_threads)) {
		thread *temp = thread_dequeue(&c->waiting_threads);
		spinlock_lock(&ready_list_lock);
		temp->state = READY;
		thread_enqueue(&ready_list, temp);
		spinlock_unlock(&ready_list_lock);
	}
	spinlock_unlock(&c->waiting_condition_lock);
}

void condition_broadcast(condition *c)
{
	spinlock_lock(&c->waiting_condition_lock);
	while (!is_empty(&c->waiting_threads)) {
		thread *temp = thread_dequeue(&c->waiting_threads);
                spinlock_lock(&ready_list_lock);
                temp->state = READY;
                thread_enqueue(&ready_list, temp);
                spinlock_unlock(&ready_list_lock);
	}
	spinlock_unlock(&c->waiting_condition_lock);
}

void spinlock_lock(AO_TS_t * lock) {
        while (AO_test_and_set_acquire(lock) == AO_TS_SET);
}

void spinlock_unlock(AO_TS_t * lock) {
        AO_CLEAR(lock);
}

void block(AO_TS_t *spinlock)
{
        spinlock_lock(&ready_list_lock);
	spinlock_unlock(spinlock);

        if (current_thread->state != DONE && current_thread->state != BLOCKED) {  
                current_thread->state = READY;
                thread_enqueue(&ready_list, current_thread);
        }

        thread *next_thread = thread_dequeue(&ready_list);
	next_thread->state = RUNNING;
          
        thread *temp = current_thread;
        set_current_thread(next_thread);

        thread_switch(temp, current_thread);
        spinlock_unlock(&ready_list_lock);
}

void scheduler_begin(int threads)
{
	/* Allocate the current_thread thread control block and set its state 
	to RUNNING. The other fields need not be initialized, at the moment*/
	thread *new_thread = (thread *) malloc(sizeof(thread));
	new_thread->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;
        mutex_init(&new_thread->mutex);
        condition_init(&new_thread->condition);
	new_thread->state = RUNNING;
	set_current_thread(new_thread);

	/* Set the head and tail fields of ready_list to NULL to indicate that 
	the ready list is empty. */
	ready_list.head = NULL;
	ready_list.tail = NULL;

	// Takes target function (fn), initial arg, region of mem (child_stack)
	int i;
	for (i = 0; i< threads; i++) {
		unsigned char *child_stack = malloc(STACK_SIZE) + STACK_SIZE; 
		int flags = CLONE_THREAD | CLONE_VM | CLONE_SIGHAND | CLONE_SIGHAND | CLONE_FILES | CLONE_IO;
		char *arg = NULL;
		clone(kernel_thread_begin, child_stack, flags, arg);
	}
}

int kernel_thread_begin(void *arg)
{
	thread *empty_thread_table = (thread *) malloc(sizeof(thread));
	mutex_init(&empty_thread_table->mutex);
        condition_init(&empty_thread_table->condition);
	empty_thread_table->state = RUNNING;
	set_current_thread(empty_thread_table);
	
	while (1) {
		yield();
	}
}

thread * thread_fork(void(*target)(void*), void *arg)
{

	// Allocate a new thread control block, and allocate its stack.
	thread *new_thread = (thread *) malloc(sizeof(thread));
	new_thread->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;
	mutex_init(&new_thread->mutex);
        condition_init(&new_thread->condition);

	// Set the new thread's initial argument and initial function.
	new_thread->initial_argument = arg;
	new_thread->initial_function = target;

	// Set the current thread's state to READY and enqueue it on the ready list.
	spinlock_lock(&ready_list_lock);
	current_thread->state = READY;
	thread_enqueue(&ready_list, current_thread);

	// Set the new thread's state to RUNNING.
	new_thread->state = RUNNING;

	/* Save a pointer to the current thread in a temporary variable, 
	then set the current thread to the new thread. */
	thread *temp = current_thread;
	set_current_thread(new_thread);

	/* Call thread_start with the old current thread as old and the 
	new current thread as new. */
	thread_start(temp, current_thread);
	spinlock_unlock(&ready_list_lock);

	return new_thread;
}

void thread_join(thread *th)
{	
        mutex_lock(&th->mutex);
	while (th->state != DONE) {
		condition_wait(&th->condition, &th->mutex);
	}
        mutex_unlock(&th->mutex);
}

void yield()
{
	spinlock_lock(&ready_list_lock);
	/* If the current thread is not DONE or BLOCKED, set its state to READY and 
	enqueue it on the ready list. */
	if (current_thread->state != DONE && current_thread->state != BLOCKED) {
		current_thread->state = READY;
		thread_enqueue(&ready_list, current_thread);
	}

	// Dequeue the next thread from the ready list and set its state to RUNNING.
	thread *next_thread = thread_dequeue(&ready_list);
	next_thread->state = RUNNING;

	/* Save a pointer to the current thread in a temporary variable, 
	then set the current thread to the next thread. */
	thread *temp = current_thread;
	set_current_thread(next_thread);
	
	/* Call thread_switch with the old current thread as old and 
	the new current thread as new. */
	thread_switch(temp, current_thread);
	spinlock_unlock(&ready_list_lock);
}

void scheduler_end()
{
	spinlock_lock(&ready_list_lock);
	while (!is_empty(&ready_list)) {
		spinlock_unlock(&ready_list_lock);
		yield();
		spinlock_lock(&ready_list_lock);
	}
	spinlock_unlock(&ready_list_lock);
}

void thread_wrap()
{
	spinlock_unlock(&ready_list_lock);
        current_thread->initial_function(current_thread->initial_argument);
	mutex_lock(&current_thread->mutex);
	current_thread->state = DONE;
	mutex_unlock(&current_thread->mutex);
	condition_signal(&current_thread->condition);
	yield();
}
