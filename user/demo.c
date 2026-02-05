// demo.c - Demo program for MLFQ visualization
// Creates mixed CPU-bound and I/O-bound processes for demonstration
// Run this alongside mlfqmon to see MLFQ in action

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void cpu_worker(int id) {
  for (int iter = 0; iter < 5; iter++) {
    // CPU-intensive work
    volatile long result = 0;
    for (long i = 0; i < 30000; i++) {
      result += i * i;
      for (int j = 0; j < 30; j++) {
        result ^= (result << 2) | (result >> 3);
      }
    }
  }
  exit(0);
}

void io_worker(int id) {
  for (int iter = 0; iter < 8; iter++) {
    // Small work then sleep (simulates I/O)
    volatile int sum = 0;
    for (int i = 0; i < 100; i++) {
      sum += i;
    }
    sleep(5);  // I/O wait
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  int num_cpu = 2;
  int num_io = 2;
  
  printf("MLFQ Demo - Creating workload\n");
  printf("  %d CPU-bound processes (will be demoted)\n", num_cpu);
  printf("  %d I/O-bound processes (will stay high priority)\n", num_io);
  printf("\nRun 'mlfqmon' in another terminal to observe!\n\n");
  
  sleep(10);
  
  // Create CPU-bound processes
  for (int i = 0; i < num_cpu; i++) {
    int pid = fork();
    if (pid == 0) {
      cpu_worker(i);
    }
  }
  
  // Create I/O-bound processes
  for (int i = 0; i < num_io; i++) {
    int pid = fork();
    if (pid == 0) {
      io_worker(i);
    }
  }
  
  // Wait for all children
  int status;
  for (int i = 0; i < num_cpu + num_io; i++) {
    wait(&status);
  }
  
  printf("\nDemo completed! All workers finished.\n");
  exit(0);
}
