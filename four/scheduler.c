#include "scheduler.h"
#include <stdio.h>

thread *current_thread;
struct queue ready_list;

// Initialize mutex
// 0 == unlocked 1 == locked
void mutex_init(mutex *m)
{
        m->held = 0;
        m->waiting_threads.head = NULL;
        m->waiting_threads.tail = NULL;
}

void mutex_lock(mutex *m)
{
        if (m->held == 0) {
                m->held = 1;
        } else {
                // Lock is held, block current thread
                current_thread->state = BLOCKED;
                thread_enqueue(&m->waiting_threads, current_thread);
		yield();
        }
}

void mutex_unlock(mutex *m)
{
	if (!is_empty(&m->waiting_threads)) {
		thread *temp;
		temp = thread_dequeue(&m->waiting_threads);
		temp->state = READY;
		thread_enqueue(&ready_list, temp);
	} else {
		m->held = 0;	
	}
}

void condition_init(condition *c) 
{
	c->waiting_threads.head = NULL;
	c->waiting_threads.tail = NULL;
}

void condition_wait(condition *c, mutex *m) 
{
	mutex_unlock(m);
	current_thread->state = BLOCKED;
	thread_enqueue(&c->waiting_threads, current_thread);
	yield();
	mutex_lock(m);		
}
	
void condition_signal(condition *c)
{
	thread *temp;
	temp = thread_dequeue(&c->waiting_threads);
	temp->state = READY;
	thread_enqueue(&ready_list, temp);	
}

void condition_broadcast(condition *c)
{
	while (!is_empty(&c->waiting_threads)) {
		condition_signal(c);
	}
}

void scheduler_begin()
{
	/* Allocate the current_thread thread control block and set its state 
	to RUNNING. The other fields need not be initialized, at the moment*/
	current_thread = (thread *) malloc(sizeof(thread));
	current_thread->state = RUNNING;

	/* Set the head and tail fields of ready_list to NULL to indicate that 
	the ready list is empty. */
	ready_list.head = NULL;
	ready_list.tail = NULL;
}

thread * thread_fork(void(*target)(void*), void *arg)
{
	// Allocate a new thread control block, and allocate its stack.
	thread *new_thread = (thread *) malloc(sizeof(thread));
	new_thread->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;

	// Set the new thread's initial argument and initial function.
	new_thread->initial_argument = arg;
	new_thread->initial_function = target;

	// Set the current thread's state to READY and enqueue it on the ready list.
	current_thread->state = READY;
	thread_enqueue(&ready_list, current_thread);

	// Set the new thread's state to RUNNING.
	new_thread->state = RUNNING;

	/* Save a pointer to the current thread in a temporary variable, 
	then set the current thread to the new thread. */
	thread *temp = current_thread;
	current_thread = new_thread;

	/* Call thread_start with the old current thread as old and the 
	new current thread as new. */
	thread_start(temp, current_thread);

	return current_thread;
}

void thread_join(thread *th)
{	
//        while (th->state != DONE) {
	if (th->state != DONE) {
                condition_wait(&current_thread->condition, &current_thread->mutex);
        }
}

void yield()
{
	/* If the current thread is not DONE or BLOCKED, set its state to READY and 
	enqueue it on the ready list. */
	if (current_thread->state != DONE && current_thread->state != BLOCKED) {
		current_thread->state = READY;
		thread_enqueue(&ready_list, current_thread);
	}

	// Dequeue the next thread from the ready list and set its state to RUNNING.
	thread *next_thread = thread_dequeue(&ready_list);

	/* Save a pointer to the current thread in a temporary variable, 
	then set the current thread to the next thread. */
	thread *temp = current_thread;
	current_thread = next_thread;
	
	/* Call thread_switch with the old current thread as old and 
	the new current thread as new. */
	thread_switch(temp, current_thread);
}

void scheduler_end()
{
	while (!is_empty(&ready_list)) {
		yield();
	}
}

void thread_wrap()
{
        current_thread->initial_function(current_thread->initial_argument);
	current_thread->state = DONE;
	condition_broadcast(&current_thread->condition);
	yield();
}
