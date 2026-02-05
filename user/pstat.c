// pstat.c - Display process scheduler statistics
// Shows MLFQ priority and tick information for all processes

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

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

static char *states[] = {
  "UNUSED",
  "USED",
  "SLEEP",
  "RUNBL",
  "RUN",
  "ZOMBI"
};

int main(int argc, char *argv[]) {
  struct procinfo pinfo[NPROC];
  
  if (getpinfo(pinfo) < 0) {
    printf("getpinfo failed\n");
    exit(1);
  }
  
  printf("\n");
  printf("        MLFQ Scheduler Process Statistics\n");
  printf("PID\tPRIO\tSTATE\tTICKS\tTOTAL\tNAME\n");
  
  int count = 0;
  int queue_count[3] = {0, 0, 0};
  
  for (int i = 0; i < NPROC; i++) {
    if (pinfo[i].inuse) {
      char *state = "???";
      if (pinfo[i].state >= 0 && pinfo[i].state <= 5) {
        state = states[pinfo[i].state];
      }
      
      printf("%d\t%d\t%s\t%d\t%d\t%s\n",
             pinfo[i].pid,
             pinfo[i].priority,
             state,
             pinfo[i].ticks_used,
             pinfo[i].ticks_total,
             pinfo[i].name);
      
      count++;
      if (pinfo[i].priority >= 0 && pinfo[i].priority < 3) {
        queue_count[pinfo[i].priority]++;
      }
    }
  }
  
  printf("Total processes: %d\n", count);
  printf("\nQueue distribution:\n");
  printf("  Queue 0 (HIGH):   %d processes\n", queue_count[0]);
  printf("  Queue 1 (MEDIUM): %d processes\n", queue_count[1]);
  printf("  Queue 2 (LOW):    %d processes\n", queue_count[2]);
  
  exit(0);
}
