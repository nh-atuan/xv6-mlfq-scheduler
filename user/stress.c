// stress.c - Stress test tool for MLFQ Scheduler
// Creates CPU-bound and I/O-bound processes to demonstrate MLFQ behavior
// Usage: stress [num_cpu] [num_io] [duration]

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

// CPU-intensive worker
void cpu_worker(int id, int duration)
{
  int start = uptime();
  volatile long result = 0;
  
  while(uptime() - start < duration) {
    // Heavy computation - will exhaust time slice
    for(long i = 0; i < 50000; i++) {
      result += i * i;
      for(int j = 0; j < 50; j++) {
        result ^= (result << 2) | (result >> 3);
      }
    }
  }
  
  exit(0);
}

// I/O-bound worker (frequent sleeps)
void io_worker(int id, int duration)
{
  int start = uptime();
  
  while(uptime() - start < duration) {
    // Small amount of work
    volatile int sum = 0;
    for(int i = 0; i < 500; i++) {
      sum += i;
    }
    // Sleep to simulate I/O wait - gives up CPU voluntarily
    sleep(3);
  }
  
  exit(0);
}

// Mixed worker - alternates between CPU and I/O
void mixed_worker(int id, int duration)
{
  int start = uptime();
  int phase = 0;
  
  while(uptime() - start < duration) {
    if(phase % 2 == 0) {
      // CPU phase
      volatile long result = 0;
      for(long i = 0; i < 20000; i++) {
        result += i * i;
      }
    } else {
      // I/O phase
      sleep(5);
    }
    phase++;
  }
  
  exit(0);
}

int main(int argc, char *argv[])
{
  int num_cpu = 2;      // Default: 2 CPU-bound
  int num_io = 2;       // Default: 2 I/O-bound
  int duration = 200;   // Default: 200 ticks (~20 seconds)
  
  // Parse arguments
  if(argc > 1) {
    num_cpu = atoi(argv[1]);
    if(num_cpu < 0) num_cpu = 0;
    if(num_cpu > 5) num_cpu = 5;
  }
  if(argc > 2) {
    num_io = atoi(argv[2]);
    if(num_io < 0) num_io = 0;
    if(num_io > 5) num_io = 5;
  }
  if(argc > 3) {
    duration = atoi(argv[3]);
    if(duration < 50) duration = 50;
    if(duration > 1000) duration = 1000;
  }
  
  printf("\n");
  printf("========================================\n");
  printf("     MLFQ Stress Test Starting\n");
  printf("========================================\n");
  printf("  CPU-bound workers: %d\n", num_cpu);
  printf("  I/O-bound workers: %d\n", num_io);
  printf("  Duration: %d ticks\n", duration);
  printf("\n");
  printf("Expected behavior:\n");
  printf("  - CPU-bound: Will be DEMOTED (Q0 -> Q1 -> Q2)\n");
  printf("  - I/O-bound: Will STAY at Q0 (high priority)\n");
  printf("========================================\n\n");
  
  printf("Run 'monitor' in another shell to observe!\n\n");
  sleep(30);  // Give user time to start monitor
  
  int total = 0;
  
  // Create CPU-bound workers
  for(int i = 0; i < num_cpu; i++) {
    int pid = fork();
    if(pid == 0) {
      // Child process
      cpu_worker(i, duration);
    } else if(pid > 0) {
      total++;
      printf("Started CPU worker %d (PID %d)\n", i, pid);
    }
  }
  
  // Create I/O-bound workers
  for(int i = 0; i < num_io; i++) {
    int pid = fork();
    if(pid == 0) {
      // Child process
      io_worker(i, duration);
    } else if(pid > 0) {
      total++;
      printf("Started I/O worker %d (PID %d)\n", i, pid);
    }
  }
  
  printf("\nWaiting for all workers to complete...\n\n");
  
  // Wait for all children
  for(int i = 0; i < total; i++) {
    int status;
    wait(&status);
  }
  
  printf("========================================\n");
  printf("     Stress Test Completed!\n");
  printf("========================================\n");
  
  exit(0);
}
