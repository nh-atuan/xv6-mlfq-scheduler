// schedtest.c - Comprehensive MLFQ scheduler test
// Creates multiple CPU-bound and I/O-bound processes to demonstrate
// how MLFQ handles different workload types.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define NUM_CPU_PROCS 2
#define NUM_IO_PROCS 2

// Process info structure (must match kernel's structure)
struct procinfo {
  int inuse;
  int pid;
  int priority;
  int state;
  int ticks_used;
  int ticks_total;
  char name[16];
};

// Helper to find process priority by PID
int get_priority(int target_pid) {
  struct procinfo pinfo[NPROC];
  if (getpinfo(pinfo) < 0) return -1;
  
  for (int i = 0; i < NPROC; i++) {
    if (pinfo[i].inuse && pinfo[i].pid == target_pid) {
      return pinfo[i].priority;
    }
  }
  return -1;
}

// CPU-intensive work
volatile long cpu_result;

void cpu_work(int id, int iterations) {
  int pid = getpid();
  int start_prio = get_priority(pid);
  printf("[CPU-%d PID %d] Starting CPU-bound work (Priority: %d)\n", id, pid, start_prio);
  
  for (int i = 0; i < iterations; i++) {
    volatile long result = 0;
    // Heavy computation - volatile prevents optimization
    // 2M iterations should take multiple ticks
    for (long j = 0; j < 2000000; j++) {
      result += j * j;
      result ^= (result << 3) | (result >> 5);
      result += (result % 7) * (result % 13);
    }
    cpu_result = result;
    int curr_prio = get_priority(pid);
    printf("[CPU-%d PID %d] Iter %d done (Priority: %d)\n", id, pid, i + 1, curr_prio);
  }
  
  int end_prio = get_priority(pid);
  printf("[CPU-%d PID %d] Finished (Priority: %d -> %d)\n", id, pid, start_prio, end_prio);
}

// I/O-bound work (frequent sleeps)
void io_work(int id, int iterations) {
  int pid = getpid();
  int start_prio = get_priority(pid);
  printf("[IO-%d PID %d] Starting I/O-bound work (Priority: %d)\n", id, pid, start_prio);
  
  for (int i = 0; i < iterations; i++) {
    // Small amount of work
    int sum = 0;
    for (int j = 0; j < 500; j++) {
      sum += j;
    }
    // Sleep to simulate I/O wait
    sleep(2);
    int curr_prio = get_priority(pid);
    printf("[IO-%d PID %d] Iter %d done (Priority: %d)\n", id, pid, i + 1, curr_prio);
  }
  
  int end_prio = get_priority(pid);
  printf("[IO-%d PID %d] Finished (Priority: %d -> %d)\n", id, pid, start_prio, end_prio);
}

int main(int argc, char *argv[]) {
  int cpu_iters = 4;
  int io_iters = 6;
  
  printf("\n");
  printf("=============================================================\n");
  printf("           MLFQ Scheduler Test\n");
  printf("=============================================================\n");
  printf("Creating %d CPU-bound and %d I/O-bound processes.\n\n", 
         NUM_CPU_PROCS, NUM_IO_PROCS);
  printf("Expected behavior:\n");
  printf("  - CPU-bound processes: DEMOTED to lower priority (0->1->2)\n");
  printf("  - I/O-bound processes: STAY in high priority (keep at 0)\n");
  printf("  - I/O processes should complete faster (more responsive)\n");
  printf("=============================================================\n\n");

  int start_time = uptime();
  int status;

  // Phase 1: Create and run CPU-bound processes sequentially
  printf("--- PHASE 1: CPU-bound processes (should demote) ---\n\n");
  for (int i = 0; i < NUM_CPU_PROCS; i++) {
    int pid = fork();
    if (pid == 0) {
      // Child - CPU-bound work
      cpu_work(i, cpu_iters);
      exit(0);
    }
    // Wait for this child to finish before starting next
    wait(&status);
    printf("\n");
  }

  // Phase 2: Create and run I/O-bound processes sequentially
  printf("--- PHASE 2: I/O-bound processes (should stay at priority 0) ---\n\n");
  for (int i = 0; i < NUM_IO_PROCS; i++) {
    int pid = fork();
    if (pid == 0) {
      // Child - I/O-bound work
      io_work(i, io_iters);
      exit(0);
    }
    // Wait for this child to finish before starting next
    wait(&status);
    printf("\n");
  }

  int end_time = uptime();
  
  printf("=============================================================\n");
  printf("   TEST COMPLETED!\n");
  printf("   Total execution time: %d ticks\n", end_time - start_time);
  printf("=============================================================\n");
  printf("\nKey observations:\n");
  printf("  - CPU-bound processes should show priority degradation (0->1->2)\n");
  printf("  - I/O-bound processes should maintain priority 0\n");
  printf("  - This demonstrates MLFQ's adaptive scheduling\n\n");

  exit(0);
}
