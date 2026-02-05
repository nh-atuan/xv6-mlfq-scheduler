// setpri.c - Set process priority manually
// Usage: setpri <pid> <priority>
// Priority: 0 (highest) to 2 (lowest)

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: setpri <pid> <priority>\n");
    printf("  priority: 0 (HIGH), 1 (MEDIUM), 2 (LOW)\n");
    exit(1);
  }
  
  int pid = atoi(argv[1]);
  int priority = atoi(argv[2]);
  
  if (priority < 0 || priority > 2) {
    printf("Error: priority must be 0, 1, or 2\n");
    exit(1);
  }
  
  if (setpriority(pid, priority) < 0) {
    printf("Error: failed to set priority for PID %d\n", pid);
    exit(1);
  }
  
  printf("Set PID %d to priority %d\n", pid, priority);
  exit(0);
}
