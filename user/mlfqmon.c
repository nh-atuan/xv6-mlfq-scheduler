// mlfqmon.c - Real-time MLFQ Scheduler Monitor
// Displays live scheduler queue status with visual representation
// Usage: mlfqmon [refresh_interval_in_ticks]

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
  "USED  ",
  "SLEEP ",
  "RUNBLE",
  "RUN   ",
  "ZOMBIE"
};

// Clear screen by printing newlines
void clear_screen(void) {
  for (int i = 0; i < 25; i++) {
    printf("\n");
  }
}

// Draw a simple bar
void draw_bar(int count, int max, char c) {
  int bar_width = 20;
  int filled = (max > 0) ? (count * bar_width / max) : 0;
  if (count > 0 && filled == 0) filled = 1;  // At least 1 char if count > 0
  
  printf("[");
  for (int i = 0; i < bar_width; i++) {
    if (i < filled) {
      printf("%c", c);
    } else {
      printf(" ");
    }
  }
  printf("]");
}

int main(int argc, char *argv[]) {
  struct procinfo pinfo[NPROC];
  int interval = 10;  // Default: refresh every 10 ticks
  int iterations = 0;
  int max_iterations = 50;  // Run for ~50 refreshes then exit
  
  if (argc > 1) {
    interval = atoi(argv[1]);
    if (interval < 1) interval = 1;
  }
  if (argc > 2) {
    max_iterations = atoi(argv[2]);
  }

  printf("MLFQ Monitor started. Refresh every %d ticks.\n", interval);
  printf("Press Ctrl+C or wait for %d iterations to exit.\n\n", max_iterations);
  sleep(20);  // Give user time to read

  while (iterations < max_iterations) {
    // Get process info
    if (getpinfo(pinfo) < 0) {
      printf("getpinfo failed\n");
      exit(1);
    }

    // Count processes in each queue
    int queue_count[3] = {0, 0, 0};
    int total = 0;
    int running = 0;
    int sleeping = 0;
    
    for (int i = 0; i < NPROC; i++) {
      if (pinfo[i].inuse && pinfo[i].state != 0) {  // Not UNUSED
        total++;
        if (pinfo[i].priority >= 0 && pinfo[i].priority < 3) {
          queue_count[pinfo[i].priority]++;
        }
        if (pinfo[i].state == 4) running++;      // RUNNING
        if (pinfo[i].state == 2) sleeping++;     // SLEEPING
      }
    }

    // Clear and draw header
    clear_screen();
    printf("        MLFQ SCHEDULER MONITOR (Refresh #%d)\n", iterations + 1);
    printf("Time: %d ticks\n\n", uptime());

    // Draw queue visualization
    printf("QUEUE STATUS:\n");
    
    printf("Queue 0 [HIGH  ] (%d) ", queue_count[0]);
    draw_bar(queue_count[0], total > 0 ? total : 1, '#');
    printf("\n");
    
    printf("Queue 1 [MEDIUM] (%d) ", queue_count[1]);
    draw_bar(queue_count[1], total > 0 ? total : 1, '=');
    printf("\n");
    
    printf("Queue 2 [LOW   ] (%d) ", queue_count[2]);
    draw_bar(queue_count[2], total > 0 ? total : 1, '-');
    printf("\n");
    
    printf("------------------------------------------------------------\n\n");

    // Process table
    printf("PROCESS TABLE:\n");
    printf("PID\tPRIO\tSTATE\tTICKS\tTOTAL\tNAME\n");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < NPROC; i++) {
      if (pinfo[i].inuse && pinfo[i].state != 0) {
        char *state = "???";
        if (pinfo[i].state >= 0 && pinfo[i].state <= 5) {
          state = states[pinfo[i].state];
        }
        
        // Highlight running process
        if (pinfo[i].state == 4) {
          printf("*%d\t%d\t%s\t%d\t%d\t%s*\n",
                 pinfo[i].pid,
                 pinfo[i].priority,
                 state,
                 pinfo[i].ticks_used,
                 pinfo[i].ticks_total,
                 pinfo[i].name);
        } else {
          printf("%d\t%d\t%s\t%d\t%d\t%s\n",
                 pinfo[i].pid,
                 pinfo[i].priority,
                 state,
                 pinfo[i].ticks_used,
                 pinfo[i].ticks_total,
                 pinfo[i].name);
        }
      }
    }
    
    printf("Total: %d | Running: %d | Sleeping: %d\n", total, running, sleeping);
    
    // Wait before next refresh
    sleep(interval);
    iterations++;
  }

  printf("\nMonitor finished after %d iterations.\n", iterations);
  exit(0);
}
