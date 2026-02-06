// monitor.c - MLFQ Scheduler Monitor TUI
// Real-time visualization of Multi-Level Feedback Queue scheduler
// Usage: monitor [iterations] [refresh_interval]
// Note: xv6 doesn't handle Ctrl+C. Wait for iterations to finish or use Ctrl+A X to exit QEMU.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

// ============================================================================
// ANSI Escape Codes for TUI
// ============================================================================
#define ANSI_CLEAR       "\033[2J"      // Clear entire screen
#define ANSI_HOME        "\033[H"       // Move cursor to top-left
#define ANSI_HIDE_CURSOR "\033[?25l"    // Hide cursor
#define ANSI_SHOW_CURSOR "\033[?25h"    // Show cursor

// Colors
#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_DIM         "\033[2m"

#define ANSI_RED         "\033[31m"
#define ANSI_GREEN       "\033[32m"
#define ANSI_YELLOW      "\033[33m"
#define ANSI_BLUE        "\033[34m"
#define ANSI_MAGENTA     "\033[35m"
#define ANSI_CYAN        "\033[36m"
#define ANSI_WHITE       "\033[37m"

#define ANSI_BG_RED      "\033[41m"
#define ANSI_BG_GREEN    "\033[42m"
#define ANSI_BG_YELLOW   "\033[43m"
#define ANSI_BG_BLUE     "\033[44m"

// ============================================================================
// Constants
// ============================================================================
#define BAR_WIDTH        20
#define MAX_DISPLAY_PROC 15

// ============================================================================
// State name lookup
// ============================================================================
static char *state_names[] = {
  "UNUSED",
  "USED",
  "SLEEP",
  "READY",
  "RUN",
  "ZOMBIE"
};

// ============================================================================
// Helper Functions for Formatted Output (xv6 printf doesn't support width)
// ============================================================================

// Get number of digits in an integer
int num_digits(int n)
{
  if(n == 0) return 1;
  int count = 0;
  if(n < 0) { n = -n; count++; }
  while(n > 0) { count++; n /= 10; }
  return count;
}

// Print integer right-aligned in given width
void print_int_r(int val, int width)
{
  int digits = num_digits(val);
  for(int i = 0; i < width - digits; i++)
    printf(" ");
  printf("%d", val);
}

// Print integer left-aligned in given width  
void print_int_l(int val, int width)
{
  printf("%d", val);
  int digits = num_digits(val);
  for(int i = 0; i < width - digits; i++)
    printf(" ");
}

// Get string length
int str_len(char *s)
{
  int len = 0;
  while(s[len]) len++;
  return len;
}

// Print string left-aligned in given width
void print_str_l(char *s, int width)
{
  printf("%s", s);
  int len = str_len(s);
  for(int i = 0; i < width - len; i++)
    printf(" ");
}

// Print string right-aligned in given width
void print_str_r(char *s, int width)
{
  int len = str_len(s);
  for(int i = 0; i < width - len; i++)
    printf(" ");
  printf("%s", s);
}

// ============================================================================
// Drawing Functions
// ============================================================================

// Draw a horizontal line
void draw_line(int width, char c)
{
  for(int i = 0; i < width; i++)
    printf("%c", c);
}

// Draw a progress bar [████░░░░]
void draw_bar(int filled, int total, char fill_char, char empty_char)
{
  int fill_width = 0;
  if(total > 0) {
    fill_width = (filled * BAR_WIDTH) / total;
    if(filled > 0 && fill_width == 0) fill_width = 1;  // At least 1 if any
  }
  
  printf("[");
  for(int i = 0; i < BAR_WIDTH; i++) {
    if(i < fill_width)
      printf("%c", fill_char);
    else
      printf("%c", empty_char);
  }
  printf("]");
}

// Draw time slice progress bar [███░]
void draw_timeslice_bar(int used, int total)
{
  int bar_len = 4;  // Fixed 4 chars
  int filled = (total > 0) ? (used * bar_len / total) : 0;
  if(used > 0 && filled == 0) filled = 1;
  if(filled > bar_len) filled = bar_len;
  
  printf("[");
  for(int i = 0; i < bar_len; i++) {
    if(i < filled)
      printf("#");
    else
      printf(".");
  }
  printf("]");
}

// Clear screen (fallback without ANSI)
void clear_screen_simple(void)
{
  for(int i = 0; i < 30; i++)
    printf("\n");
}

// ============================================================================
// Main Drawing Functions
// ============================================================================

void draw_header(int iteration, int global_ticks)
{
  printf(ANSI_BOLD);
  printf("+");
  draw_line(80, '-');
  printf("+\n");
  
  printf("|");
  print_str_l("              MLFQ SCHEDULER MONITOR v1.0", 80);
  printf("|\n");
  
  // System Time line
  printf("|         System Time: ");
  printf("%d", global_ticks);
  printf(" ticks");
  int len = num_digits(global_ticks);
  for(int i = 0; i < 80 - 28 - len; i++) printf(" ");
  printf("|\n");
  
  // Refresh line
  printf("|");
  for(int i = 0; i < 56; i++) printf(" ");
  printf("Refresh: #");
  printf("%d", iteration);
  len = num_digits(iteration);
  for(int i = 0; i < 80 - 67 - len; i++) printf(" ");
  printf("|\n");
  
  printf("+");
  draw_line(80, '-');
  printf("+\n");
  printf(ANSI_RESET);
}

void draw_queue_section(struct pstat *ps)
{
  int max_count = ps->sys.total_processes;
  if(max_count < 1) max_count = 1;
  
  printf(ANSI_BOLD "\n  QUEUE VISUALIZATION");
  printf("                         SYSTEM STATS\n" ANSI_RESET);
  printf("  ");
  draw_line(21, '=');
  printf("                         ");
  draw_line(12, '=');
  printf("\n\n");
  
  // Queue 0 - HIGH
  printf("  Q0 " ANSI_GREEN "[HIGH  ]" ANSI_RESET " ");
  draw_bar(ps->sys.queue_count[0], max_count, '#', ' ');
  printf("  %d procs", ps->sys.queue_count[0]);
  printf("     Total:     %d processes\n", ps->sys.total_processes);
  
  printf("              Time Slice: 1 tick");
  printf("                Running:    %d\n", ps->sys.running_count);
  
  // Queue 1 - MEDIUM
  printf("  Q1 " ANSI_YELLOW "[MEDIUM]" ANSI_RESET " ");
  draw_bar(ps->sys.queue_count[1], max_count, '=', ' ');
  printf("  %d procs", ps->sys.queue_count[1]);
  printf("     Sleeping:  %d\n", ps->sys.sleeping_count);
  
  printf("              Time Slice: 2 ticks");
  printf("               Runnable:   %d\n", ps->sys.runnable_count);
  
  // Queue 2 - LOW
  printf("  Q2 " ANSI_RED "[LOW   ]" ANSI_RESET " ");
  draw_bar(ps->sys.queue_count[2], max_count, '-', ' ');
  printf("  %d procs", ps->sys.queue_count[2]);
  printf("     \n");
  
  printf("              Time Slice: 4 ticks");
  printf("               Next Boost: %d ticks\n", ps->sys.next_boost_in);
  
  printf("\n");
}

void draw_process_table(struct pstat *ps)
{
  printf("+");
  draw_line(80, '-');
  printf("+\n");
  
  printf(ANSI_BOLD ANSI_WHITE);
  printf("  PROCESS TABLE\n");
  printf(ANSI_RESET);
  printf("  ");
  draw_line(13, '=');
  printf("\n\n");
  
  // Header - Bold White
  printf(ANSI_BOLD ANSI_WHITE);
  printf("  PID  | NAME         | STATE   | PRIO | SLICE  | TOTAL | SCHED | DEM | BST\n");
  printf(ANSI_RESET);
  printf("  -----+--------------+---------+------+--------+-------+-------+-----+----\n");
  
  // Process rows
  int displayed = 0;
  for(int i = 0; i < PSTAT_NPROC && displayed < MAX_DISPLAY_PROC; i++) {
    if(ps->procs[i].inuse && ps->procs[i].state != PSTAT_UNUSED) {
      char *state = "???   ";
      int prio = ps->procs[i].priority;
      int is_running = (ps->procs[i].state == PSTAT_RUNNING);
      
      if(ps->procs[i].state >= 0 && ps->procs[i].state <= 5) {
        state = state_names[ps->procs[i].state];
      }
      
      // Color based on state and priority
      // Priority: RUNNING (cyan) > Q0/HIGH (green) > Q1/MED (yellow) > Q2/LOW (red)
      char *row_color = ANSI_RESET;
      
      if(is_running) {
        // Running process = Cyan (highest priority visual)
        row_color = ANSI_BOLD ANSI_CYAN;
      } else if(prio == 0) {
        // Queue 0 (HIGH priority) = Green
        row_color = ANSI_GREEN;
      } else if(prio == 1) {
        // Queue 1 (MEDIUM priority) = Yellow
        row_color = ANSI_YELLOW;
      } else if(prio == 2) {
        // Queue 2 (LOW priority) = Red
        row_color = ANSI_RED;
      }
      
      // Start row with color
      printf("%s", row_color);
      
      // Running indicator
      if(is_running) {
        printf(" *");
      } else {
        printf("  ");
      }
      
      // PID (4 chars right-aligned)
      print_int_r(ps->procs[i].pid, 4);
      printf(" | ");
      
      // NAME (12 chars left-aligned)
      print_str_l(ps->procs[i].name, 12);
      printf(" | ");
      
      // STATE (7 chars left-aligned)
      print_str_l(state, 7);
      printf(" | ");
      
      // PRIO (4 chars center)
      printf("  %d ", prio);
      printf(" | ");
      
      // SLICE bar
      draw_timeslice_bar(ps->procs[i].ticks_current, ps->procs[i].time_slice);
      printf(" | ");
      
      // TOTAL (5 chars right-aligned)
      print_int_r(ps->procs[i].ticks_total, 5);
      printf(" | ");
      
      // SCHED (5 chars right-aligned)
      print_int_r(ps->procs[i].num_scheduled, 5);
      printf(" | ");
      
      // DEM (3 chars right-aligned)
      print_int_r(ps->procs[i].num_demoted, 3);
      printf(" | ");
      
      // BST (3 chars right-aligned)
      print_int_r(ps->procs[i].num_boosted, 3);
      
      // Reset color at end of row
      printf(ANSI_RESET "\n");
      
      displayed++;
    }
  }
  
  printf("\n");
}

void draw_footer(int interval)
{
  printf("+");
  draw_line(80, '-');
  printf("+\n");
  printf("  Legend: " ANSI_CYAN "*=RUN" ANSI_RESET " | ");
  printf(ANSI_GREEN "Q0=HIGH" ANSI_RESET " | ");
  printf(ANSI_YELLOW "Q1=MED" ANSI_RESET " | ");
  printf(ANSI_RED "Q2=LOW" ANSI_RESET " | ");
  printf("Refresh: ");
  printf("%d", interval);
  printf(" ticks\n");
  printf("  Wait for iterations to finish (xv6 doesn't support Ctrl+C)\n");
  printf("+");
  draw_line(80, '-');
  printf("+\n");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char *argv[])
{
  struct pstat *ps;
  int interval = 10;      // Default: refresh every 10 ticks
  int max_iter = 100;     // Default: run 100 iterations
  int use_ansi = 1;       // Try ANSI codes by default
  
  // Parse arguments: monitor [iterations] [interval]
  if(argc > 1) {
    max_iter = atoi(argv[1]);
    if(max_iter < 1) max_iter = 1;
  }
  if(argc > 2) {
    interval = atoi(argv[2]);
    if(interval < 1) interval = 1;
    if(interval > 100) interval = 100;
  }
  if(argc > 3) {
    // Third argument: 0 = no ANSI, 1 = use ANSI
    use_ansi = atoi(argv[3]);
  }
  
  // Allocate pstat on heap (struct is ~4KB, too large for stack!)
  ps = (struct pstat*)malloc(sizeof(struct pstat));
  if(ps == 0) {
    printf("monitor: malloc failed\n");
    exit(1);
  }
  
  printf(ANSI_CLEAR ANSI_HOME);
  printf("MLFQ Monitor starting...\n");
  printf("  Iterations: %d\n", max_iter);
  printf("  Refresh interval: %d ticks\n", interval);
  printf("  ANSI colors: %s\n", use_ansi ? "enabled" : "disabled");
  printf("\nNote: xv6 doesn't support Ctrl+C. Wait for iterations or Ctrl+A X to exit QEMU.\n");
  printf("\nStarting in 2 seconds...\n");
  sleep(20);  // ~2 seconds
  
  // Hide cursor for cleaner display
  if(use_ansi) {
    printf(ANSI_HIDE_CURSOR);
  }
  
  // Main loop
  for(int iter = 1; iter <= max_iter; iter++) {
    // Get process info
    if(getpstat(ps) < 0) {
      printf("monitor: getpstat failed\n");
      free(ps);
      exit(1);
    }
    
    // Clear and redraw
    if(use_ansi) {
      printf(ANSI_HOME);  // Move to top instead of clearing (less flicker)
    } else {
      clear_screen_simple();
    }
    
    // Draw UI
    draw_header(iter, ps->sys.global_ticks);
    draw_queue_section(ps);
    draw_process_table(ps);
    draw_footer(interval);
    
    // Wait for next refresh
    sleep(interval);
  }
  
  // Show cursor again
  if(use_ansi) {
    printf(ANSI_SHOW_CURSOR);
  }
  
  printf("\nMonitor finished after %d iterations.\n", max_iter);
  free(ps);
  exit(0);
}
