// pstat.h - Enhanced process statistics for MLFQ Monitor TUI
// Shared between kernel and user space
// Replaces the older pinfo.h with richer data

#ifndef _PSTAT_H_
#define _PSTAT_H_

#define PSTAT_NPROC     64    // Must match NPROC in param.h
#define PSTAT_NAME_LEN  16    // Max process name length

// Process states (matching enum procstate in proc.h)
#define PSTAT_UNUSED    0
#define PSTAT_USED      1
#define PSTAT_SLEEPING  2
#define PSTAT_RUNNABLE  3
#define PSTAT_RUNNING   4
#define PSTAT_ZOMBIE    5

// Per-process snapshot
struct proc_stat {
  int     inuse;              // Whether this slot is in use
  int     pid;                // Process ID
  int     ppid;               // Parent Process ID
  int     state;              // Process state (UNUSED..ZOMBIE)
  int     priority;           // Current MLFQ queue (0=HIGH, 1=MED, 2=LOW)

  // Time accounting
  int     ticks_current;      // Ticks used in current time slice
  int     ticks_total;        // Total ticks used since creation
  int     time_slice;         // Time slice length for current queue (1/2/4)

  // Scheduling history
  int     num_scheduled;      // Number of times scheduled
  int     num_demoted;        // Number of times demoted
  int     num_boosted;        // Number of times boosted

  char    name[PSTAT_NAME_LEN]; // Process name
};

// System-wide MLFQ statistics
struct mlfq_stat {
  int     global_ticks;       // Current system ticks (uptime)
  int     last_boost_tick;    // Tick when last priority boost occurred
  int     next_boost_in;      // Ticks remaining until next boost
  int     queue_count[3];     // Number of processes in each queue
  int     total_processes;    // Total active processes
  int     running_count;      // Number of RUNNING processes
  int     sleeping_count;     // Number of SLEEPING processes
  int     runnable_count;     // Number of RUNNABLE processes
};

// Complete system snapshot returned by getpstat() syscall
struct pstat {
  struct mlfq_stat  sys;                    // System-wide stats
  struct proc_stat  procs[PSTAT_NPROC];     // Per-process stats
};

#endif // _PSTAT_H_
