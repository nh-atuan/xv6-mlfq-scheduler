// test_pstat.c - Test program for the getpstat() syscall
// Verifies that struct pstat is correctly populated from kernel

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  struct pstat *ps;

  printf("Starting getpstat() test...\n");

  // Allocate pstat on heap to avoid stack overflow (struct is ~4KB)
  ps = (struct pstat*)malloc(sizeof(struct pstat));
  if(ps == 0) {
    printf("ERROR: malloc failed\n");
    exit(1);
  }

  int ret = getpstat(ps);
  if(ret < 0) {
    printf("ERROR: getpstat() returned %d\n", ret);
    free(ps);
    exit(1);
  }

  printf("SUCCESS: getpstat() returned 0\n");

  // Print system-wide stats
  printf("\nSYSTEM STATS:\n");
  printf("  Global ticks:     %d\n", ps->sys.global_ticks);
  printf("  Total processes:  %d\n", ps->sys.total_processes);
  printf("  Queue 0 (HIGH):   %d\n", ps->sys.queue_count[0]);
  printf("  Queue 1 (MED):    %d\n", ps->sys.queue_count[1]);
  printf("  Queue 2 (LOW):    %d\n", ps->sys.queue_count[2]);

  printf("\nTest PASSED\n");
  free(ps);
  exit(0);
}
