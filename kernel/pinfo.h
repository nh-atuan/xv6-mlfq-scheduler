// pinfo.h - Process info structure for MLFQ scheduler
// Shared between kernel and user space

#ifndef _PINFO_H_
#define _PINFO_H_

#include "kernel/param.h"

// Process info structure returned by getpinfo syscall
struct pinfo {
  int inuse[NPROC];         // Whether this slot is in use
  int pid[NPROC];           // PID of each process
  int priority[NPROC];      // Current priority queue (0=highest)
  int state[NPROC];         // Process state
  int ticks_used[NPROC];    // Ticks used in current time slice
  int ticks_total[NPROC];   // Total ticks used
  char name[NPROC][16];     // Process name
};

#endif // _PINFO_H_
