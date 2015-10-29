#include "scheduler.h"
#include <stdio.h>

void print_nth_prime(void * pn) {
printf("In function\n");
  int n = *(int *) pn;
  int c = 1, i = 1;
  while(c <= n) {
    ++i;
    int j, isprime = 1;
    for(j = 2; j < i; ++j) {
	if(i % j == 0) {
        isprime = 0;
        break;
      }
    }
    if(isprime) {
      ++c;
    }
printf("Right before yield\n");
    yield();
  }
  printf("%dth prime: %d\n", n, i);
 
}


int main(void) {
  scheduler_begin();

  int n1 = 7, n2 = 10000, n3 = 30000;
printf("FIRST THREAD FORK\n");
  thread_fork(print_nth_prime, &n1);
printf("SECOND THREAD FORK\n");
  thread_fork(print_nth_prime, &n2);
printf("THIRD THREAD FORK\n");
  thread_fork(print_nth_prime, &n3);

  scheduler_end();
}

