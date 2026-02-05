// schedtest.c - Comprehensive MLFQ scheduler test
// Creates multiple CPU-bound and I/O-bound processes to demonstrate
// how MLFQ handles different workload types.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_CPU_PROCS 2
#define NUM_IO_PROCS 2

// CPU-intensive work
void cpu_work(int id, int iterations) {
  int pid = getpid();
  printf("[CPU-%d PID %d] Starting CPU-bound work\n", id, pid);
  
  for (int i = 0; i < iterations; i++) {
    volatile long result = 0;
    // Heavy computation
    for (long j = 0; j < 50000; j++) {
      result += j * j;
      for (int k = 0; k < 50; k++) {
        result ^= (result << 2) | (result >> 3);
      }
    }
    printf("[CPU-%d PID %d] Iter %d done\n", id, pid, i + 1);
  }
  
  printf("[CPU-%d PID %d] Finished\n", id, pid);
}

// I/O-bound work (frequent sleeps)
void io_work(int id, int iterations) {
  int pid = getpid();
  printf("[IO-%d PID %d] Starting I/O-bound work\n", id, pid);
  
  for (int i = 0; i < iterations; i++) {
    // Small amount of work
    int sum = 0;
    for (int j = 0; j < 500; j++) {
      sum += j;
    }
    // Sleep to simulate I/O wait
    sleep(3);
    printf("[IO-%d PID %d] Iter %d done\n", id, pid, i + 1);
  }
  
  printf("[IO-%d PID %d] Finished\n", id, pid);
}

int main(int argc, char *argv[]) {
  int cpu_iters = 3;
  int io_iters = 5;
  
  printf("   MLFQ Scheduler Test\n");
  printf("Creating %d CPU-bound and %d I/O-bound processes.\n\n", 
         NUM_CPU_PROCS, NUM_IO_PROCS);
  printf("Expected behavior:\n");
  printf("- CPU-bound processes: DEMOTED to lower priority\n");
  printf("- I/O-bound processes: STAY in high priority\n");
  printf("- I/O processes should complete faster (more responsive)\n");

  int start_time = uptime();

  // Create CPU-bound processes
  for (int i = 0; i < NUM_CPU_PROCS; i++) {
    int pid = fork();
    if (pid == 0) {
      // Child - CPU-bound work
      cpu_work(i, cpu_iters);
      exit(0);
    }
  }

  // Create I/O-bound processes
  for (int i = 0; i < NUM_IO_PROCS; i++) {
    int pid = fork();
    if (pid == 0) {
      // Child - I/O-bound work
      io_work(i, io_iters);
      exit(0);
    }
  }

  // Wait for all children
  int status;
  for (int i = 0; i < NUM_CPU_PROCS + NUM_IO_PROCS; i++) {
    wait(&status);
  }

  int end_time = uptime();
  
  printf("   All processes completed!\n");
  printf("   Total time: %d ticks\n", end_time - start_time);

  exit(0);
}
