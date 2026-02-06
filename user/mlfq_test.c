// mlfq_test.c - Automated MLFQ Scheduler Test Suite
// Runs comprehensive tests and verifies MLFQ behavior automatically
// Usage: mlfq_test

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define TEST_PASSED  0
#define TEST_FAILED  1

// Test results tracking
static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

// Helper: Print test header
void test_header(char *name)
{
  printf("\n");
  printf("============================================================\n");
  printf("  Test: %s\n", name);
  printf("============================================================\n");
}

// Helper: Print test result
void test_result(char *test_name, int passed, char *reason)
{
  total_tests++;
  if(passed) {
    passed_tests++;
    printf("  [PASS] %s\n", test_name);
  } else {
    failed_tests++;
    printf("  [FAIL] %s\n", test_name);
    if(reason)
      printf("           Reason: %s\n", reason);
  }
}

// Helper: Get process priority by PID
int get_priority(int pid)
{
  struct pstat *ps = malloc(sizeof(struct pstat));
  if(!ps) return -1;
  
  if(getpstat(ps) < 0) {
    free(ps);
    return -1;
  }
  
  for(int i = 0; i < PSTAT_NPROC; i++) {
    if(ps->procs[i].inuse && ps->procs[i].pid == pid) {
      int prio = ps->procs[i].priority;
      free(ps);
      return prio;
    }
  }
  
  free(ps);
  return -1;
}

// Helper: Get demote count by PID
int get_demote_count(int pid)
{
  struct pstat *ps = malloc(sizeof(struct pstat));
  if(!ps) return -1;
  
  if(getpstat(ps) < 0) {
    free(ps);
    return -1;
  }
  
  for(int i = 0; i < PSTAT_NPROC; i++) {
    if(ps->procs[i].inuse && ps->procs[i].pid == pid) {
      int count = ps->procs[i].num_demoted;
      free(ps);
      return count;
    }
  }
  
  free(ps);
  return -1;
}

// ============================================================================
// Test 1: Syscall Functionality
// ============================================================================
int test_syscall(void)
{
  test_header("Syscall Functionality");
  
  struct pstat *ps = malloc(sizeof(struct pstat));
  if(!ps) {
    test_result("malloc allocation", TEST_FAILED, "malloc failed");
    return TEST_FAILED;
  }
  
  // Test 1.1: getpstat returns success
  int ret = getpstat(ps);
  test_result("getpstat() returns 0", ret == 0, "syscall failed");
  if(ret != 0) {
    free(ps);
    return TEST_FAILED;
  }
  
  // Test 1.2: System stats are populated
  test_result("global_ticks > 0", ps->sys.global_ticks > 0, "ticks not updated");
  test_result("total_processes > 0", ps->sys.total_processes > 0, "no processes found");
  
  // Test 1.3: At least init and sh should be present
  int found_processes = 0;
  for(int i = 0; i < PSTAT_NPROC; i++) {
    if(ps->procs[i].inuse && ps->procs[i].state != PSTAT_UNUSED) {
      found_processes++;
    }
  }
  test_result("found processes >= 2", found_processes >= 2, "expected init + sh");
  
  free(ps);
  return TEST_PASSED;
}

// ============================================================================
// Test 2: Priority Demotion (CPU-bound)
// ============================================================================
int test_priority_demotion(void)
{
  test_header("Priority Demotion (CPU-bound)");
  
  int pid = fork();
  if(pid < 0) {
    test_result("fork()", TEST_FAILED, "fork failed");
    return TEST_FAILED;
  }
  
  if(pid == 0) {
    // Child: CPU-intensive work
    volatile long result = 0;
    for(int iter = 0; iter < 10; iter++) {
      for(long i = 0; i < 50000; i++) {
        result += i * i;
        for(int j = 0; j < 30; j++) {
          result ^= (result << 2);
        }
      }
    }
    exit(0);
  }
  
  // Parent: Monitor priority changes
  sleep(5);  // Let child start
  int start_prio = get_priority(pid);
  
  sleep(20); // Let it run and demote
  int mid_prio = get_priority(pid);
  
  sleep(20); // More time
  int end_prio = get_priority(pid);
  
  // Wait for child
  int status;
  wait(&status);
  
  // Verify demotion happened
  test_result("initial priority = 0", start_prio == 0, "should start at HIGH");
  test_result("priority decreased", end_prio > start_prio, "CPU-bound should demote");
  
  int demote_count = get_demote_count(pid);
  test_result("demote count > 0", demote_count > 0, "CPU-bound should be demoted");
  
  printf("  Details: Priority %d -> %d -> %d, demoted %d times\n",
         start_prio, mid_prio, end_prio, demote_count);
  
  return (end_prio > start_prio) ? TEST_PASSED : TEST_FAILED;
}

// ============================================================================
// Test 3: Priority Preservation (I/O-bound)
// ============================================================================
int test_priority_preservation(void)
{
  test_header("Priority Preservation (I/O-bound)");
  
  int pid = fork();
  if(pid < 0) {
    test_result("fork()", TEST_FAILED, "fork failed");
    return TEST_FAILED;
  }
  
  if(pid == 0) {
    // Child: I/O-bound (frequent sleeps)
    for(int i = 0; i < 15; i++) {
      volatile int sum = 0;
      for(int j = 0; j < 100; j++) {
        sum += j;
      }
      sleep(3);  // Voluntary yield
    }
    exit(0);
  }
  
  // Parent: Monitor priority
  sleep(5);
  int start_prio = get_priority(pid);
  
  sleep(25);
  int mid_prio = get_priority(pid);
  
  sleep(25);
  int end_prio = get_priority(pid);
  
  int status;
  wait(&status);
  
  // Verify priority stayed high
  test_result("initial priority = 0", start_prio == 0, "should start at HIGH");
  test_result("priority stayed at 0", end_prio == 0, "I/O-bound should stay HIGH");
  
  int demote_count = get_demote_count(pid);
  test_result("demote count = 0", demote_count == 0, "I/O-bound should not demote");
  
  printf("  Details: Priority %d -> %d -> %d, demoted %d times\n",
         start_prio, mid_prio, end_prio, demote_count);
  
  return (end_prio == 0 && demote_count == 0) ? TEST_PASSED : TEST_FAILED;
}

// ============================================================================
// Test 4: Mixed Workload Fairness
// ============================================================================
int test_mixed_fairness(void)
{
  test_header("Mixed Workload Fairness");
  
  int cpu_pid = fork();
  if(cpu_pid == 0) {
    // CPU-bound child
    volatile long result = 0;
    for(long i = 0; i < 200000; i++) {
      result += i * i;
    }
    exit(0);
  }
  
  int io_pid = fork();
  if(io_pid == 0) {
    // I/O-bound child
    for(int i = 0; i < 20; i++) {
      sleep(2);
    }
    exit(0);
  }
  
  // Track completion times
  int first_done = -1;
  int start = uptime();
  
  for(int i = 0; i < 2; i++) {
    int status;
    int done_pid = wait(&status);
    int elapsed = uptime() - start;
    
    if(first_done < 0) {
      first_done = done_pid;
      printf("  First finished: PID %d at %d ticks\n", done_pid, elapsed);
    } else {
      printf("  Second finished: PID %d at %d ticks\n", done_pid, elapsed);
    }
  }
  
  // I/O-bound should finish first (more responsive)
  test_result("I/O finished before CPU", first_done == io_pid, 
              "I/O-bound should be more responsive");
  
  return (first_done == io_pid) ? TEST_PASSED : TEST_FAILED;
}

// ============================================================================
// Test 5: Queue Distribution
// ============================================================================
int test_queue_distribution(void)
{
  test_header("Queue Distribution");
  
  struct pstat *ps = malloc(sizeof(struct pstat));
  if(!ps) {
    test_result("malloc", TEST_FAILED, "allocation failed");
    return TEST_FAILED;
  }
  
  // Create CPU-bound workers
  for(int i = 0; i < 3; i++) {
    int pid = fork();
    if(pid == 0) {
      volatile long result = 0;
      for(long j = 0; j < 100000; j++) {
        result += j * j;
      }
      exit(0);
    }
  }
  
  sleep(30); // Let them run and distribute
  
  if(getpstat(ps) < 0) {
    free(ps);
    test_result("getpstat", TEST_FAILED, "syscall failed");
    return TEST_FAILED;
  }
  
  // Check queue distribution
  printf("  Queue distribution:\n");
  printf("    Q0: %d processes\n", ps->sys.queue_count[0]);
  printf("    Q1: %d processes\n", ps->sys.queue_count[1]);
  printf("    Q2: %d processes\n", ps->sys.queue_count[2]);
  
  int total_in_queues = ps->sys.queue_count[0] + 
                        ps->sys.queue_count[1] + 
                        ps->sys.queue_count[2];
  
  test_result("processes distributed", total_in_queues > 0, "no processes in queues");
  test_result("at least 2 queues used", 
              (ps->sys.queue_count[0] > 0) + (ps->sys.queue_count[1] > 0) + (ps->sys.queue_count[2] > 0) >= 2,
              "CPU workers should spread across queues");
  
  // Clean up
  for(int i = 0; i < 3; i++) {
    int status;
    wait(&status);
  }
  
  free(ps);
  return TEST_PASSED;
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main(void)
{
  printf("\n");
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("║          MLFQ SCHEDULER - AUTOMATED TEST SUITE              ║\n");
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("\n");
  printf("Running comprehensive tests...\n");
  
  // Run all tests
  test_syscall();
  test_priority_demotion();
  test_priority_preservation();
  test_mixed_fairness();
  test_queue_distribution();
  
  // Print summary
  printf("\n");
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("║                      TEST SUMMARY                            ║\n");
  printf("╠══════════════════════════════════════════════════════════════╣\n");
  printf("║                                                              ║\n");
  printf("║  Total Tests:   %2d                                           ║\n", total_tests);
  printf("║  Passed:        %2d                                            ║\n", passed_tests);
  printf("║  Failed:        %2d                                            ║\n", failed_tests);
  printf("║                                                              ║\n");
  
  if(failed_tests == 0) {
    printf("║  Result:        ALL TESTS PASSED!                           ║\n");
  } else {
    printf("║  Result:        SOME TESTS FAILED                           ║\n");
  }
  
  printf("║                                                              ║\n");
  printf("╚══════════════════════════════════════════════════════════════╝\n");
  printf("\n");
  
  exit(failed_tests > 0 ? 1 : 0);
}
