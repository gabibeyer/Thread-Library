#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

#define STACK_SIZE 1024*1024

struct Thread
{
        unsigned char* stack_pointer;
        void (*initial_function)(void*);
        void* initial_argument;
} typedef Thread;

Thread *current_thread;
Thread *inactive_thread;

void thread_switch(Thread *old, Thread *new);
void thread_start(Thread *old, Thread *new);

void yield()
{
        Thread *temp = current_thread;
        current_thread = inactive_thread;
        inactive_thread = temp;
        thread_switch(inactive_thread, current_thread);
}

void thread_wrap()
{
        current_thread->initial_function(current_thread->initial_argument);
	yield();
}

bool is_even(int n)
{
        return n % 2 == 0 ? true : false;
}
void writing_void_func(void* arg)
{
        int n = *(int*) arg;
        if (is_even(n)) {
                printf("%d is even\n", n);
        } else {
                printf("%d is odd\n", n);
        }
}

int main()
{
        // Allocating space for current_thread and inactive_thread 
        current_thread = (Thread *) malloc(sizeof(Thread));
        inactive_thread = (Thread *) malloc(sizeof(Thread));

        // Initialize initial function in current thread
        current_thread->initial_function = writing_void_func;

        // Initialize initial arguement in current thread
        int *p = malloc(sizeof(int));
        *p = 10;
        current_thread->initial_argument = p;

        // Initialize stack_pointer in current thread
        current_thread->stack_pointer = malloc(STACK_SIZE) + STACK_SIZE;

        thread_start(inactive_thread, current_thread);

return 0;
}

