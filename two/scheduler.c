#include "scheduler.h"
#include "queue.h"
#include <stdio.h>

thread *current_thread;
struct queue ready_list;

void scheduler_begin()
{
printf("Scheduler init\n");

	// Allocate the current_thread thread control block and set its state 
	// to RUNNING. The other fields need not be initialized; at the moment, 
	current_thread = (thread *) malloc(sizeof(thread));
	current_thread->state = RUNNING;
printf("INIT: Current Thread is running\n");

	/* Set the head and tail fields of ready_list to NULL to indicate that 
	the ready list is empty. */
	ready_list.head = NULL;
	ready_list.tail = NULL;
}

void thread_fork(void(*target)(void*), void *arg)
{
	// Allocate a new thread control block, and allocate its stack.
	thread *new_thread = (thread *) malloc(sizeof(thread));
	new_thread->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;
printf("FORK: New Thread allocated\n");

	// Set the new thread's initial argument and initial function.
	new_thread->initial_argument = arg;
	new_thread->initial_function = target;

	// Set the current thread's state to READY and enqueue it on the ready list.
	current_thread->state = READY;
	thread_enqueue(&ready_list, current_thread);
printf("FORK: Placed current thread onto queue\n");

	// Set the new thread's state to RUNNING.
	new_thread->state = RUNNING;

	/* Save a pointer to the current thread in a temporary variable, 
	then set the current thread to the new thread. */
	thread *temp = current_thread;
	current_thread = new_thread;

	/* Call thread_start with the old current thread as old and the 
	new current thread as new. */
printf("FORK: New current thread running\n");
	thread_start(temp, current_thread);
}

void yield()
{
	/* If the current thread is not DONE, set its state to READY and 
	enqueue it on the ready list. */
	if (current_thread->state != DONE) {
		current_thread->state = READY;
		thread_enqueue(&ready_list, current_thread);
printf("YIELD: current thread not finished, back on queue\n");
	} else {
		printf("Current thread finished running\n");
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
printf("YIELD: Running dequeued thread\n");
}

void scheduler_end()
{
//	if (!is_empty(&ready_list)) {
//		printf("not empty\n");
//	}
	while (!is_empty(&ready_list)) {
		yield();
	}
}

void thread_wrap()
{
        current_thread->initial_function(current_thread->initial_argument);
	current_thread->state = DONE;
	yield();
}
