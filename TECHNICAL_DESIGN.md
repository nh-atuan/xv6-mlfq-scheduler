# 📋 Technical Design Document: MLFQ Monitor TUI

**Version:** 1.1  
**Date:** February 6, 2026  
**Status:** ✅ Approved - Ready for Implementation

---

## 1. Tổng quan kiến trúc

```
┌─────────────────────────────────────────────────────────────────┐
│                        USER SPACE                               │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │  monitor.c  │    │  stress.c   │    │ workload.c  │         │
│  │   (TUI)     │    │ (Test Tool) │    │ (Generator) │         │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘         │
│         │                  │                  │                 │
│         └──────────────────┼──────────────────┘                 │
│                            │                                    │
│                   ┌────────▼────────┐                           │
│                   │  struct pstat   │  ← Data Transfer Object   │
│                   │   (pstat.h)     │                           │
│                   └────────┬────────┘                           │
├────────────────────────────┼────────────────────────────────────┤
│                            │  SYSTEM CALL BOUNDARY              │
│                 ┌──────────▼──────────┐                         │
│                 │   sys_getpstat()    │                         │
│                 └──────────┬──────────┘                         │
├────────────────────────────┼────────────────────────────────────┤
│                        KERNEL SPACE                             │
│                            │                                    │
│              ┌─────────────▼─────────────┐                      │
│              │  MLFQ Scheduler (proc.c)  │                      │
│              │  ┌─────┐ ┌─────┐ ┌─────┐  │                      │
│              │  │ Q0  │ │ Q1  │ │ Q2  │  │                      │
│              │  │HIGH │ │MED  │ │LOW  │  │                      │
│              │  └─────┘ └─────┘ └─────┘  │                      │
│              └───────────────────────────┘                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Thiết kế Data Structure

### 2.1 `struct pstat` - Giao thức trao đổi Kernel ↔ User

**File:** `kernel/pstat.h` (mới, thay thế pinfo.h)

```c
#ifndef _PSTAT_H_
#define _PSTAT_H_

#define PSTAT_NPROC     64    // NPROC
#define PSTAT_NAME_LEN  16    // Max process name length

// Process states (matching enum procstate)
#define PSTAT_UNUSED    0
#define PSTAT_USED      1
#define PSTAT_SLEEPING  2
#define PSTAT_RUNNABLE  3
#define PSTAT_RUNNING   4
#define PSTAT_ZOMBIE    5

// Single process snapshot
struct proc_stat {
  int     inuse;              // Slot đang được sử dụng?
  int     pid;                // Process ID
  int     ppid;               // Parent Process ID (NEW)
  int     state;              // UNUSED/SLEEPING/RUNNABLE/RUNNING/ZOMBIE
  int     priority;           // Current MLFQ queue (0=HIGH, 1=MED, 2=LOW)
  
  // Time accounting
  int     ticks_current;      // Ticks đã dùng trong time slice hiện tại
  int     ticks_total;        // Tổng ticks đã sử dụng từ khi tạo process
  int     time_slice;         // Time slice của queue hiện tại (NEW)
  
  // Scheduling history (NEW)
  int     num_scheduled;      // Số lần được scheduled
  int     num_demoted;        // Số lần bị demote
  int     num_boosted;        // Số lần được boost
  
  char    name[PSTAT_NAME_LEN]; // Process name
};

// System-wide MLFQ statistics (NEW)
struct mlfq_stat {
  uint64  global_ticks;       // uptime() - system ticks
  int     last_boost_tick;    // Tick của lần boost gần nhất
  int     next_boost_in;      // Còn bao nhiêu tick đến lần boost tiếp
  int     queue_count[3];     // Số process trong mỗi queue
  int     total_processes;    // Tổng số process đang active
  int     running_count;      // Số process đang RUNNING
  int     sleeping_count;     // Số process đang SLEEPING
  int     runnable_count;     // Số process đang RUNNABLE
};

// Complete system snapshot returned by syscall
struct pstat {
  struct mlfq_stat  sys;                    // System-wide stats
  struct proc_stat  procs[PSTAT_NPROC];     // Per-process stats
};

#endif // _PSTAT_H_
```

### 2.2 Tính toán kích thước struct

```c
sizeof(struct proc_stat)     ≈ 60 bytes
sizeof(struct mlfq_stat)     ≈ 40 bytes
sizeof(struct pstat)         = 40 + (64 * 60) = 3,880 bytes ≈ 4KB

⚠️  WARNING: Gần bằng kernel stack size (4KB) → KHÔNG được khai báo trên stack!
```

### 2.3 Giải thích các trường

| Trường | Mục đích | Nguồn dữ liệu |
|--------|----------|---------------|
| `pid`, `ppid` | Xác định process và quan hệ cha-con | `p->pid`, `p->parent->pid` |
| `state` | Hiển thị trạng thái process | `p->state` |
| `priority` | Queue hiện tại (0/1/2) | `p->priority` |
| `ticks_current` | Progress bar trong time slice | `p->ticks_used` |
| `ticks_total` | Thống kê CPU usage | `p->ticks_total` |
| `time_slice` | Time slice của queue (1/2/4) | Lookup từ `MLFQ_TICKS_x` |
| `num_demoted` | Chứng minh CPU-bound bị demote | Cần thêm field vào `struct proc` |
| `next_boost_in` | Countdown đến priority boost | `BOOST_INTERVAL - (ticks % BOOST_INTERVAL)` |

---

## 3. Thiết kế Kernel Backend

### 3.1 System Call: `sys_getpstat`

**Signature:**
```c
int getpstat(struct pstat *ps);
```

**Input:** Con trỏ user-space đến buffer `struct pstat`  
**Output:** 0 nếu thành công, -1 nếu lỗi  

### 3.2 Flow của System Call

```
┌──────────────────────────────────────────────────────────────────┐
│                    sys_getpstat() Flow                           │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  1. Lấy user address từ a0  │
                │     argaddr(0, &addr)       │
                └─────────────┬───────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  2. Validate address        │
                │     (addr != 0)             │
                └─────────────┬───────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  3. Allocate kernel buffer  │
                │     kstat = kalloc()        │
                │     if(!kstat) return -1    │
                └─────────────┬───────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  4. Zero out buffer         │
                │     memset(kstat, 0, ...)   │
                └─────────────┬───────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  5. Tính toán sys stats     │
                │     - acquire(&tickslock)   │
                │     - global_ticks = ticks  │
                │     - next_boost = ...      │
                │     - release(&tickslock)   │
                └─────────────┬───────────────┘
                              │
                              ▼
          ┌───────────────────────────────────────────┐
          │  6. Loop qua proc[] table                 │
          │  ┌─────────────────────────────────────┐  │
          │  │  for(p = proc; p < &proc[NPROC])   │  │
          │  │  {                                  │  │
          │  │    acquire(&p->lock);  ◄── LOCKING │  │
          │  │                                     │  │
          │  │    // Copy fields                   │  │
          │  │    kstat->procs[i].pid = p->pid;   │  │
          │  │    kstat->procs[i].state = ...     │  │
          │  │    ...                              │  │
          │  │                                     │  │
          │  │    // Update counters              │  │
          │  │    kstat->sys.queue_count[...]++; │  │
          │  │                                     │  │
          │  │    release(&p->lock); ◄── UNLOCK   │  │
          │  │  }                                  │  │
          │  └─────────────────────────────────────┘  │
          └───────────────────┬───────────────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  7. copyout() to user space │
                │     - Copy từ kernel buffer │
                │       ra user address       │
                │     - Kiểm tra lỗi          │
                └─────────────┬───────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  8. Free kernel buffer      │
                │     kfree(kstat)            │
                └─────────────┬───────────────┘
                              │
                              ▼
                ┌─────────────────────────────┐
                │  9. Return 0 (success)      │
                └─────────────────────────────┘
```

### 3.3 Implementation với Heap Allocation

⚠️ **CRITICAL: Stack Overflow Prevention**

xv6 kernel stack chỉ có **4KB**. `struct pstat` (~4KB) sẽ gây **stack overflow** nếu khai báo local variable.

**✅ Solution: Dynamic Memory Allocation**

```c
// kernel/sysproc.c

uint64
sys_getpstat(void)
{
  uint64 user_addr;
  struct pstat *kstat;
  struct proc *p;
  int i;
  
  // 1. Lấy user space address
  argaddr(0, &user_addr);
  if(user_addr == 0)
    return -1;
  
  // 2. ★ Allocate buffer on HEAP (not stack!)
  kstat = (struct pstat*)kalloc();
  if(kstat == 0)
    return -1;  // Out of memory
  
  // 3. Zero out buffer
  memset(kstat, 0, sizeof(struct pstat));
  
  // 4. Gather system-wide statistics
  acquire(&tickslock);
  kstat->sys.global_ticks = ticks;
  kstat->sys.next_boost_in = BOOST_INTERVAL - (ticks % BOOST_INTERVAL);
  kstat->sys.last_boost_tick = ticks - (ticks % BOOST_INTERVAL);
  release(&tickslock);
  
  // 5. Gather per-process statistics
  i = 0;
  for(p = proc; p < &proc[NPROC]; p++, i++) {
    acquire(&p->lock);
    
    if(p->state != UNUSED) {
      kstat->procs[i].inuse = 1;
      kstat->procs[i].pid = p->pid;
      kstat->procs[i].ppid = (p->parent) ? p->parent->pid : 0;
      kstat->procs[i].state = p->state;
      kstat->procs[i].priority = p->priority;
      kstat->procs[i].ticks_current = p->ticks_used;
      kstat->procs[i].ticks_total = p->ticks_total;
      kstat->procs[i].num_scheduled = p->num_scheduled;
      kstat->procs[i].num_demoted = p->num_demoted;
      kstat->procs[i].num_boosted = p->num_boosted;
      
      // Determine time slice based on priority
      if(p->priority == 0)
        kstat->procs[i].time_slice = MLFQ_TICKS_0;
      else if(p->priority == 1)
        kstat->procs[i].time_slice = MLFQ_TICKS_1;
      else
        kstat->procs[i].time_slice = MLFQ_TICKS_2;
      
      // Copy process name
      memmove(kstat->procs[i].name, p->name, sizeof(p->name));
      
      // Update system counters
      kstat->sys.total_processes++;
      kstat->sys.queue_count[p->priority]++;
      
      if(p->state == RUNNING)
        kstat->sys.running_count++;
      else if(p->state == SLEEPING)
        kstat->sys.sleeping_count++;
      else if(p->state == RUNNABLE)
        kstat->sys.runnable_count++;
    }
    
    release(&p->lock);
  }
  
  // 6. Copy to user space (atomic from user's perspective)
  if(copyout(myproc()->pagetable, user_addr, 
             (char*)kstat, sizeof(struct pstat)) < 0) {
    kfree(kstat);  // ★ Don't forget to free on error!
    return -1;
  }
  
  // 7. ★ Free kernel buffer
  kfree(kstat);
  
  return 0;
}
```

### 3.4 Memory Management Best Practices

```c
┌─────────────────────────────────────────────────────────┐
│         Memory Management Pattern                       │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Allocate:     kstat = kalloc()                      │
│  2. Check NULL:   if(!kstat) return -1                  │
│  3. Initialize:   memset(kstat, 0, PGSIZE)              │
│  4. Use:          ... populate data ...                 │
│  5. Copy:         copyout(..., kstat, ...)              │
│  6. Free:         kfree(kstat)                          │
│                                                         │
│  ⚠️  ALWAYS pair kalloc() with kfree()                  │
│  ⚠️  Free on ALL error paths!                           │
│  ⚠️  Don't free before successful copyout!              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 3.5 Cơ chế an toàn (Locking)

```c
// Fine-grained locking strategy:

for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);      // ★ Lock ONE process at a time
    
    // Critical section - read process fields
    // ...
    
    release(&p->lock);      // ★ Release immediately after read
}

// Why fine-grained?
// - Reduced lock contention
// - Other CPUs can access different processes
// - Shorter critical sections = better performance
```

---

## 4. Thiết kế User Frontend (Monitor TUI)

### 4.1 File: `user/monitor.c`

### 4.2 Mockup Giao diện ASCII Art

```
┌──────────────────────────────────────────────────────────────────────────┐
│                     MLFQ SCHEDULER MONITOR v1.0                          │
│                    System Time: 1234 ticks | Refresh: #42                │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  QUEUE VISUALIZATION                           SYSTEM STATS              │
│  ═══════════════════════                       ════════════              │
│                                                                          │
│  Q0 [HIGH  ] ████████░░░░░░░░░░░░  4 procs     Total:     12 processes   │
│              Time Slice: 1 tick                Running:    1             │
│                                                Sleeping:   8             │
│  Q1 [MEDIUM] ████░░░░░░░░░░░░░░░░  2 procs     Runnable:   3             │
│              Time Slice: 2 ticks                                         │
│                                                Next Boost: 34 ticks      │
│  Q2 [LOW   ] ██████████░░░░░░░░░░  5 procs     Last Boost: 66 ticks ago  │
│              Time Slice: 4 ticks                                         │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  PROCESS TABLE                                                           │
│  ═════════════                                                           │
│                                                                          │
│  PID  PPID  NAME         STATE    PRIO  TICKS    PROGRESS     TOTAL      │
│  ───  ────  ──────────   ──────   ────  ─────    ────────     ─────      │
│ *  4     1  cpu_worker   RUNNING    2    3/4     [███░]         127      │
│    5     1  io_worker    SLEEPING   0    0/1     [░░░░]          23      │
│    6     1  cpu_worker   RUNNABLE   2    4/4     [████]          98      │
│    7     1  io_worker    SLEEPING   0    0/1     [░░░░]          19      │
│    3     2  monitor      RUNNING    0    0/1     [░░░░]          45      │
│    2     1  sh           SLEEPING   0    0/1     [░░░░]          12      │
│    1     0  init         SLEEPING   0    0/1     [░░░░]           5      │
│                                                                          │
├──────────────────────────────────────────────────────────────────────────┤
│  Legend: * = Currently Running | PRIO: 0=HIGH, 1=MED, 2=LOW              │
│  Controls: Press Ctrl+C to exit | Refresh interval: 5 ticks              │
└──────────────────────────────────────────────────────────────────────────┘
```

### 4.3 Chi tiết các thành phần UI

**A. Queue Visualization Bar:**
```
Q0 [HIGH  ] ████████░░░░░░░░░░░░  4 procs
            ^                  ^
            |                  |
            filled            empty
            (count/max)
```
- Độ dài bar: 20 ký tự
- `█` (filled) = `count * 20 / max_count`
- `░` (empty) = phần còn lại

**B. Time Slice Progress:**
```
[███░]
 ^ ^
 | |
 | ticks còn lại trong slice
 ticks đã dùng
```
- 4 ký tự cho mỗi progress bar
- Dựa trên `ticks_current / time_slice`

**C. Color Coding (ANSI):**
| Trạng thái | Color Code | Màu |
|------------|------------|-----|
| RUNNING | `\033[1;32m` | **Xanh lá đậm** |
| RUNNABLE | `\033[0;33m` | Vàng |
| SLEEPING | `\033[0;36m` | Cyan |
| ZOMBIE | `\033[0;31m` | Đỏ |

### 4.4 ANSI Escape Codes sử dụng

```c
// Screen control
#define ANSI_CLEAR       "\033[2J"      // Clear toàn màn hình
#define ANSI_HOME        "\033[H"       // Di chuyển cursor về góc trên trái
#define ANSI_HIDE_CURSOR "\033[?25l"    // Ẩn cursor (chống nháy)
#define ANSI_SHOW_CURSOR "\033[?25h"    // Hiện cursor

// Colors
#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_GREEN       "\033[32m"
#define ANSI_YELLOW      "\033[33m"
#define ANSI_CYAN        "\033[36m"
#define ANSI_RED         "\033[31m"

// Kỹ thuật refresh không nháy:
void refresh_display() {
    printf(ANSI_HOME);           // 1. Về đầu màn hình (KHÔNG clear)
    printf(ANSI_HIDE_CURSOR);    // 2. Ẩn cursor
    
    // 3. Ghi đè nội dung mới lên nội dung cũ
    draw_header();
    draw_queues();
    draw_process_table();
    draw_footer();
    
    // 4. Không cần ANSI_CLEAR vì đã ghi đè
}
```

**Lưu ý quan trọng về xv6:**
- xv6 console có thể không hỗ trợ đầy đủ ANSI codes
- Cần fallback: dùng `clear_screen()` bằng newlines nếu ANSI không hoạt động
- Test trên QEMU để xác nhận compatibility

---

## 5. Thiết kế Stress Test Tool

### 5.1 File: `user/stress.c`

### 5.2 Các loại Workload

```c
// Workload types
typedef enum {
    WORKLOAD_CPU_HEAVY,     // 100% CPU, sẽ bị demote nhanh
    WORKLOAD_CPU_BURST,     // CPU burst ngắn, demote chậm hơn
    WORKLOAD_IO_FREQUENT,   // I/O thường xuyên, giữ priority cao
    WORKLOAD_IO_RARE,       // I/O hiếm, có thể bị demote
    WORKLOAD_MIXED          // Xen kẽ CPU và I/O
} workload_type;
```

### 5.3 Test Scenarios

```
┌─────────────────────────────────────────────────────────────────┐
│                    STRESS TEST SCENARIOS                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Scenario 1: PRIORITY DEMOTION TEST                             │
│  ───────────────────────────────────                            │
│  - Tạo 3 CPU-bound processes                                    │
│  - Mong đợi: Tất cả bị demote từ Q0 → Q1 → Q2                   │
│  - Thời gian: ~10-20 ticks để demote hoàn toàn                  │
│                                                                 │
│  Scenario 2: PRIORITY PRESERVATION TEST                         │
│  ───────────────────────────────────────                        │
│  - Tạo 3 I/O-bound processes (sleep mỗi 1-2 ticks)              │
│  - Mong đợi: Tất cả giữ ở Q0 (HIGH)                             │
│  - Verification: priority == 0 sau nhiều iterations            │
│                                                                 │
│  Scenario 3: MIXED WORKLOAD FAIRNESS                            │
│  ──────────────────────────────────                             │
│  - 2 CPU-bound + 2 I/O-bound processes                          │
│  - Mong đợi: I/O processes hoàn thành trước                     │
│  - Measurement: So sánh completion time                         │
│                                                                 │
│  Scenario 4: PRIORITY BOOST TEST                                │
│  ───────────────────────────────                                │
│  - Tạo CPU-bound, đợi đến bị demote xuống Q2                    │
│  - Đợi BOOST_INTERVAL (100 ticks)                               │
│  - Verify: Process được boost về Q0                             │
│                                                                 │
│  Scenario 5: STARVATION PREVENTION                              │
│  ─────────────────────────────────                              │
│  - 1 CPU-bound (ở Q2) + 5 I/O-bound (ở Q0)                      │
│  - Verify: CPU-bound vẫn nhận được CPU time                     │
│  - Measurement: CPU-bound phải tiến triển dù chậm               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 5.4 Output Format của Stress Test

```
╔══════════════════════════════════════════════════════════════════╗
║              MLFQ STRESS TEST - RESULTS SUMMARY                  ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  Test 1: Priority Demotion                          [✓ PASSED]  ║
║    - cpu_worker (PID 4): 0 → 2 in 8 ticks                       ║
║    - cpu_worker (PID 5): 0 → 2 in 9 ticks                       ║
║    - cpu_worker (PID 6): 0 → 2 in 7 ticks                       ║
║                                                                  ║
║  Test 2: Priority Preservation                      [✓ PASSED]  ║
║    - io_worker (PID 7): stayed at 0 for 50 ticks                ║
║    - io_worker (PID 8): stayed at 0 for 50 ticks                ║
║                                                                  ║
║  Test 3: Mixed Fairness                             [✓ PASSED]  ║
║    - I/O avg completion: 23 ticks                               ║
║    - CPU avg completion: 67 ticks                               ║
║    - I/O faster by: 2.9x                                        ║
║                                                                  ║
║  Test 4: Priority Boost                             [✓ PASSED]  ║
║    - Boost observed at tick 200 (expected: 200)                 ║
║    - All processes returned to Q0                               ║
║                                                                  ║
╠══════════════════════════════════════════════════════════════════╣
║  OVERALL: 4/4 tests passed                                       ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 6. Danh sách File cần tạo/sửa

### 6.1 Files mới cần tạo

| File | Mục đích | Priority |
|------|----------|----------|
| `kernel/pstat.h` | Định nghĩa `struct pstat` mới | **P0** |
| `user/monitor.c` | TUI Monitor chính | **P1** |
| `user/stress.c` | Công cụ stress test | **P2** |

### 6.2 Files cần chỉnh sửa

| File | Thay đổi | Priority |
|------|----------|----------|
| `kernel/proc.h` | Thêm fields: `num_scheduled`, `num_demoted`, `num_boosted` | **P0** |
| `kernel/proc.c` | Implement `getpstat()`, tracking statistics, update scheduler | **P0** |
| `kernel/sysproc.c` | Implement `sys_getpstat()` với kalloc/kfree | **P0** |
| `kernel/syscall.h` | Thêm `SYS_getpstat` (hoặc reuse) | **P0** |
| `kernel/syscall.c` | Register syscall | **P0** |
| `kernel/defs.h` | Khai báo `getpstat()` | **P0** |
| `user/user.h` | Thêm `getpstat()` prototype | **P0** |
| `user/usys.pl` | Thêm syscall stub | **P0** |
| `Makefile` | Thêm `$U/_monitor` và `$U/_stress` | **P1** |

---

## 7. Implementation Priority

```
Phase 1: Foundation (Kernel) - P0
├── 1.1 Tạo kernel/pstat.h
├── 1.2 Update struct proc với tracking fields
├── 1.3 Implement sys_getpstat() với kalloc/kfree
├── 1.4 Update scheduler để track statistics
├── 1.5 Register syscall
└── 1.6 Test với simple user program

Phase 2: Basic Monitor - P1
├── 2.1 Tạo user/monitor.c skeleton
├── 2.2 Implement basic process table display
├── 2.3 Add queue visualization
└── 2.4 Add refresh loop

Phase 3: Enhanced UI - P1
├── 3.1 Add ANSI color support (with fallback)
├── 3.2 Implement progress bars
├── 3.3 Add system statistics panel
└── 3.4 Handle edge cases

Phase 4: Testing - P2
├── 4.1 Tạo user/stress.c
├── 4.2 Implement test scenarios
├── 4.3 Add result verification
└── 4.4 Documentation
```

---

## 8. Risks và Mitigation

| Risk | Impact | Mitigation | Status |
|------|--------|------------|--------|
| Stack overflow từ large struct | **CRITICAL** | ✅ Sử dụng kalloc/kfree thay vì stack | **RESOLVED** |
| ANSI codes không hoạt động | Medium | Chuẩn bị fallback mode | Planned |
| Race condition khi đọc | Medium | Fine-grained locking | Planned |
| Monitor CPU overhead | Low | Adaptive refresh rate | Planned |
| copyout failure | Low | Validate + error handling | Planned |
| Memory leak | Medium | Ensure kfree() on ALL paths | **CRITICAL** |

---

## 9. Memory Safety Checklist

```
✅  Use kalloc() for large structures (>1KB)
✅  Check kalloc() return value for NULL
✅  memset() buffer after allocation
✅  kfree() on successful path
✅  kfree() on ALL error paths
✅  No use-after-free (don't access after kfree)
✅  Pair every kalloc() with exactly one kfree()
```

---

## 10. Testing Strategy

### 10.1 Unit Tests
- [ ] `sys_getpstat()` returns correct data structure size
- [ ] kalloc/kfree không leak memory
- [ ] copyout handles invalid addresses
- [ ] Locking prevents race conditions

### 10.2 Integration Tests
- [ ] Monitor hiển thị đúng số lượng processes
- [ ] Queue counts match reality
- [ ] Progress bars reflect actual time slice usage
- [ ] Priority changes visible in real-time

### 10.3 Stress Tests
- [ ] All 5 scenarios pass
- [ ] No deadlocks under heavy load
- [ ] No memory leaks after long runs
- [ ] Scheduler maintains fairness

---

## 11. Performance Targets

| Metric | Target | Rationale |
|--------|--------|-----------|
| Syscall latency | < 1ms | Không ảnh hưởng scheduler |
| Memory overhead | < 4KB | Chỉ 1 page allocation |
| Monitor refresh | 5-10 ticks | Balance giữa real-time vs overhead |
| CPU usage (monitor) | < 5% | Không làm ảnh hưởng workload |

---

**Document Status:** ✅ Approved for Implementation  
**Last Updated:** February 6, 2026  
**Next Milestone:** Phase 1 - Kernel Foundation
