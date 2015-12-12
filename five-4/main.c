#include <stdio.h>
#include <unistd.h>

#include "scheduler.h"

#define BUF_SIZE 100000

char buf[BUF_SIZE];

void print_nth_prime(void * pn) {
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
    yield();
  }
  printf("%dth prime: %d\n", n, i);
}

void read_file(void * arg) {
  int fd = (char *) arg;
  read_wrap(fd, buf, BUF_SIZE);
  close(fd);
}

int main(void) {
  scheduler_begin();
  int n1 = 20000, n2 = 10000, n3 = 30000;
  
  thread_fork(print_nth_prime, &n1);
  thread_fork(read_file, STDIN_FILENO);
  thread_fork(print_nth_prime, &n2);
  thread_fork(print_nth_prime, &n3);

  scheduler_end();
}

