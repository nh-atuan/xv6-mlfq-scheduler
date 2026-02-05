// cpu_bound.c - CPU-intensive test program for MLFQ scheduler
// This program should be demoted to lower priority queues over time
// because it uses its entire time slice without I/O operations.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Compute-intensive function
volatile long compute_result;

long compute(long n) {
  long result = 0;
  for (long i = 0; i < n; i++) {
    result += i * i;
    // Add some more computation to burn CPU
    for (int j = 0; j < 100; j++) {
      result ^= (result << 3) | (result >> 5);
    }
  }
  return result;
}

int main(int argc, char *argv[]) {
  int iterations = 5;  // Default iterations
  int pid = getpid();
  
  if (argc > 1) {
    iterations = atoi(argv[1]);
  }

  printf("CPU-bound process (PID %d) starting with %d iterations\n", pid, iterations);
  printf("This process should be DEMOTED to lower priority queues.\n\n");

  for (int i = 0; i < iterations; i++) {
    int start = uptime();
    
    // Heavy computation - will use full time slice
    compute_result = compute(100000);
    
    int end = uptime();
    printf("[PID %d] Iteration %d: computed, took %d ticks\n", 
           pid, i + 1, end - start);
  }

  printf("\nCPU-bound process (PID %d) finished.\n", pid);
  exit(0);
}
