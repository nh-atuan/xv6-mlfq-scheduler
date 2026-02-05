// io_bound.c - I/O-bound test program for MLFQ scheduler
// This program should STAY in high priority queues because it
// frequently sleeps (gives up CPU voluntarily before time slice expires).

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int iterations = 10;  // Default iterations
  int sleep_time = 5;   // Sleep ticks between iterations
  int pid = getpid();
  
  if (argc > 1) {
    iterations = atoi(argv[1]);
  }
  if (argc > 2) {
    sleep_time = atoi(argv[2]);
  }

  printf("I/O-bound process (PID %d) starting with %d iterations\n", pid, iterations);
  printf("Sleep time: %d ticks between each iteration.\n", sleep_time);
  printf("This process should STAY in high priority queues.\n\n");

  for (int i = 0; i < iterations; i++) {
    int start = uptime();
    
    // Do a small amount of work
    int sum = 0;
    for (int j = 0; j < 1000; j++) {
      sum += j;
    }
    
    // Sleep - simulating I/O wait (gives up CPU voluntarily)
    sleep(sleep_time);
    
    int end = uptime();
    printf("[PID %d] Iteration %d: sum=%d, elapsed %d ticks\n", 
           pid, i + 1, sum, end - start);
  }

  printf("\nI/O-bound process (PID %d) finished.\n", pid);
  exit(0);
}
