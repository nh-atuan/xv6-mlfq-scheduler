# Multi-Level Feedback Queue (MLFQ) Scheduler cho xv6

## Giới thiệu

Dự án này mở rộng bộ lập lịch Round-Robin của xv6 thành **Multi-Level Feedback Queue Scheduler**, cho phép hệ điều hành tự động điều chỉnh độ ưu tiên của process dựa trên hành vi sử dụng CPU.

### Đặc điểm chính:
- **3 Priority Queues**: Queue 0 (cao nhất) → Queue 2 (thấp nhất)
- **Time slices khác nhau**: Queue 0 = 1 tick, Queue 1 = 2 ticks, Queue 2 = 4 ticks
- **CPU-bound processes**: Bị demote xuống queue thấp hơn khi dùng hết time slice
- **I/O-bound processes**: Được giữ ở queue cao vì thường xuyên sleep
- **Priority Boost**: Mỗi 100 ticks, tất cả processes được đưa về Queue 0 (chống starvation)

---

## Yêu cầu hệ thống

### Trên Windows (dùng WSL):
```bash
# Cài đặt WSL Ubuntu
wsl --install

# Trong WSL, cài đặt RISC-V toolchain
sudo apt update
sudo apt install -y gcc-riscv64-unknown-elf qemu-system-misc
```

### Trên Linux:
```bash
sudo apt install -y gcc-riscv64-unknown-elf qemu-system-misc
```

---

## Cách build và chạy

### 1. Build xv6
```bash
cd xv6-labs-2023
make clean
make qemu
```

### 2. Thoát QEMU
Nhấn `Ctrl+A` rồi `X`

---

## Các lệnh demo

### 1. Xem thông tin scheduler (Legacy)
```bash
$ pstat
```
Output:
```
        MLFQ Scheduler Process Statistics
PID     PRIO    STATE   TICKS   TOTAL   NAME
1       0       SLEEP   0       1       init
2       0       SLEEP   0       0       sh
3       0       RUN     0       0       pstat
Total processes: 3

Queue distribution:
  Queue 0 (HIGH):   3 processes
  Queue 1 (MEDIUM): 0 processes
  Queue 2 (LOW):    0 processes
```

### 2. Monitor TUI (Recommended) 🎨
**Real-time MLFQ visualization với ANSI colors**

```bash
$ monitor [refresh_ticks]

# Ví dụ: refresh mỗi 10 ticks (mặc định)
$ monitor

# Hoặc tùy chỉnh refresh rate
$ monitor 5
```

Features:
- ✓ ANSI colors: Queue visualization với progress bars  
- ✓ System statistics: Global ticks, boost info, queue counts
- ✓ Process table: Top 15 processes với color-coded states
- ✓ Real-time updates: Tự động refresh theo interval
- ✓ Graceful exit: Press Ctrl+C hoặc đợi completion

**Ví dụ output:**
```
╔════════════════════════════════════════════════════════════════╗
║           MLFQ SCHEDULER MONITOR - Real-time View              ║
╠════════════════════════════════════════════════════════════════╣
║  Global Ticks:      1234                                       ║
║  Last Boost:        1234 (0 ticks ago)                         ║
║  Next Boost in:     100 ticks                                  ║
╚════════════════════════════════════════════════════════════════╝

┌─────────────────── QUEUE STATUS ───────────────────┐
│  Q0 [HIGH  ]: 3  █████████████░░░░░░░░░░░░░░░      │
│  Q1 [MEDIUM]: 1  ████░░░░░░░░░░░░░░░░░░░░░░░░      │
│  Q2 [LOW   ]: 2  ████████░░░░░░░░░░░░░░░░░░░░      │
└─────────────────────────────────────────────────────┘

PID   PPID  STATE    PRIO  TICKS  TOTAL  SCHED  DEMOTE  NAME
1     0     SLEEP    0     0      5      10     0       init
2     1     SLEEP    0     0      3      8      0       sh
*4    2     RUN      2     3      45     50     2       cpu_work
5     2     SLEEP    0     0      2      15     0       io_work
```

### 3. Stress Testing Workload 💪
**Tạo workload để test scheduler**

```bash
$ stress [cpu_workers] [io_workers] [mixed_workers] [duration_seconds]

# Ví dụ: 2 CPU-bound + 3 I/O-bound, chạy 30 giây
$ stress 2 3 0 30

# Mặc định: 2 CPU + 2 I/O + 1 mixed, chạy 20 giây
$ stress
```

Workload types:
- **CPU-bound**: Heavy computation → demote to low queues
- **I/O-bound**: Frequent sleep → stay in high queue  
- **Mixed**: Combination of CPU and I/O

### 4. Automated Testing Suite ✅
**Comprehensive MLFQ verification**

```bash
$ mlfq_test
```

Test scenarios:
1. **Syscall Functionality**: Verify getpstat() works correctly
2. **Priority Demotion**: CPU-bound processes demote to lower queues
3. **Priority Preservation**: I/O-bound processes stay at high priority
4. **Mixed Fairness**: I/O-bound finishes before CPU-bound
5. **Queue Distribution**: Processes spread across multiple queues

Output sample:
```
╔══════════════════════════════════════════════════════════════╗
║          MLFQ SCHEDULER - AUTOMATED TEST SUITE              ║
╔══════════════════════════════════════════════════════════════╗

============================================================
  Test: Priority Demotion (CPU-bound)
============================================================
  [✓ PASS] initial priority = 0
  [✓ PASS] priority decreased
  [✓ PASS] demote count > 0
  Details: Priority 0 -> 1 -> 2, demoted 2 times

╔══════════════════════════════════════════════════════════════╗
║                      TEST SUMMARY                            ║
╠══════════════════════════════════════════════════════════════╣
║  Total Tests:   15                                           ║
║  Passed:        15  ✓                                        ║
║  Failed:         0  ✗                                        ║
║  Result:        ALL TESTS PASSED! 🎉                        ║
╚══════════════════════════════════════════════════════════════╝
```

### 5. Set priority thủ công
```bash
$ setpri <pid> <priority>

# Ví dụ: Set PID 3 xuống priority 2 (LOW)
$ setpri 3 2
```

---

## Demo đầy đủ

### Demo 1: Automated Testing (Recommended) ✨
**Verify MLFQ correctness trước khi demo**

```bash
# Trong xv6
$ mlfq_test

# Đợi kết quả (~1 phút)
# Expected: ALL TESTS PASSED!
```

### Demo 2: Monitor + Stress (Visual Demo) 🎬
**Real-time visualization của scheduler**

```bash
# Bước 1: Chạy monitor
$ monitor 5 &

# Bước 2 (Option A): Stress test cân bằng
$ stress 2 2 1 25

# Bước 2 (Option B): CPU-heavy workload
$ stress 5 1 0 25

# Bước 2 (Option C): I/O-heavy workload
$ stress 1 5 0 25

# Quan sát:
# - CPU-bound processes demote từ Q0 → Q2
# - I/O-bound processes giữ ở Q0
# - Queue bars thay đổi theo thời gian
# - Priority boost mỗi 100 ticks
```

### Demo 3: So sánh CPU-bound vs I/O-bound

Chạy trong xv6:
```bash
$ schedtest
```

Kết quả mong đợi:
- **I/O-bound processes** hoàn thành nhanh hơn (được ưu tiên)
- **CPU-bound processes** bị demote và chạy sau

### Demo 4: Test riêng từng loại process

```bash
# CPU-bound (sẽ bị demote)
$ cpu_bound 10

# I/O-bound (sẽ giữ priority cao)
$ io_bound 10 5
```

### Demo 5: Legacy monitor

```bash
# Chạy monitor cũ (không có ANSI colors)
$ mlfqmon 3 50 &

# Chạy demo workload
$ demo
```

---

## Giải thích output Monitor TUI

### Header Section
```
╔════════════════════════════════════════════════════════════════╗
║           MLFQ SCHEDULER MONITOR - Real-time View              ║
╠════════════════════════════════════════════════════════════════╣
║  Global Ticks:      1234    ← Tổng ticks từ khi boot           ║
║  Last Boost:        1200    ← Lần boost cuối cùng              ║
║  Next Boost in:     66      ← Còn bao nhiêu ticks tới boost    ║
╚════════════════════════════════════════════════════════════════╝
```

### Queue Status (Visual Bars)
```
┌─────────────────── QUEUE STATUS ───────────────────┐
│  Q0 [HIGH  ]: 3  █████████████░░░░░░░░░░░░░░░      │  ← 3 processes
│  Q1 [MEDIUM]: 1  ████░░░░░░░░░░░░░░░░░░░░░░░░      │  ← 1 process  
│  Q2 [LOW   ]: 2  ████████░░░░░░░░░░░░░░░░░░░░      │  ← 2 processes
└─────────────────────────────────────────────────────┘
```
- **█**: Filled portion (số lượng processes)
- **░**: Empty portion
- Bar length tương ứng với count tương đối

### Process Table
```
PID   PPID  STATE    PRIO  TICKS  TOTAL  SCHED  DEMOTE  NAME
------------------------------------------------------------
1     0     SLEEP    0     0      5      10     0       init
2     1     SLEEP    0     0      3      8      0       sh
*4    2     RUN      2     3      45     50     2       cpu_work
5     2     SLEEP    0     0      2      15     0       io_work
```

Giải thích các cột:
- **PID**: Process ID (dấu `*` = đang RUN)
- **PPID**: Parent Process ID
- **STATE**: UNUSED/USED/SLEEP/RUNBLE/RUN/ZOMBIE
- **PRIO**: Priority level (0=HIGH, 1=MEDIUM, 2=LOW)
- **TICKS**: Ticks đã dùng trong time slice hiện tại
- **TOTAL**: Tổng ticks đã sử dụng
- **SCHED**: Số lần được scheduled
- **DEMOTE**: Số lần bị demote

### Color Coding (ANSI Terminals)
- **GREEN**: High priority processes (Queue 0)
- **YELLOW**: Medium priority (Queue 1)
- **RED**: Low priority (Queue 2)
- **BOLD**: Currently running process

Ký hiệu quan trọng:
- `*...*`: Process đang RUN (có CPU)
- `PRIO 0`: Highest priority (Q0)
- `PRIO 2`: Lowest priority (Q2)
- `DEMOTE > 0`: Process đã bị giảm priority

### Fallback Mode (Non-ANSI)
Nếu terminal không hỗ trợ ANSI, monitor tự động chuyển sang plain text mode:
```
============================================================
        MLFQ SCHEDULER MONITOR (Refresh #5)
============================================================
Time: 234 ticks

QUEUE STATUS:
------------------------------------------------------------
Queue 0 [HIGH  ] (2) [####                ]
Queue 1 [MEDIUM] (1) [==                  ]
Queue 2 [LOW   ] (2) [----                ]
------------------------------------------------------------
```

---

## Cấu trúc source code

### Kernel Implementation
```
kernel/
├── pstat.h         # Data structures for getpstat() syscall
│                   # - struct proc_stat: Per-process statistics
│                   # - struct mlfq_stat: System-wide statistics  
│                   # - struct pstat: Combined data transfer object (~4KB)
├── param.h         # MLFQ constants (NMLFQ, time slices, boost interval)
├── proc.h          # Process struct với MLFQ fields:
│                   # - priority: Current queue (0=HIGH, 2=LOW)
│                   # - ticks_used: Ticks in current time slice
│                   # - num_scheduled, num_demoted, num_boosted
├── proc.c          # Core scheduler implementation:
│                   # - scheduler(): MLFQ scheduling logic
│                   # - priority_boost(): Move all to Q0 every 100 ticks
│                   # - getpstat(): Syscall backend (uses kalloc/kfree)
├── sysproc.c       # Syscalls: sys_getpstat, sys_setpriority
├── syscall.h/c     # Syscall numbers và registration
└── defs.h          # Function declarations
```

### User Space Tools
```
user/
├── monitor.c       # ⭐ NEW: Real-time TUI với ANSI colors
│                   # - ANSI escape codes for colors/formatting
│                   # - Progress bars for queue visualization
│                   # - Auto-refresh với configurable interval
│                   # - Fallback mode for non-ANSI terminals
│                   # Usage: monitor [refresh_ticks]
│
├── stress.c        # ⭐ NEW: Workload generator
│                   # - CPU-bound workers (heavy computation)
│                   # - I/O-bound workers (frequent sleep)
│                   # - Mixed workers (combination)
│                   # Usage: stress [cpu] [io] [mixed] [seconds]
│
├── mlfq_test.c     # ⭐ NEW: Automated test suite
│                   # Test 1: Syscall functionality
│                   # Test 2: Priority demotion (CPU-bound)
│                   # Test 3: Priority preservation (I/O-bound)
│                   # Test 4: Mixed fairness
│                   # Test 5: Queue distribution
│                   # Usage: mlfq_test
│
├── pstat.c         # Legacy: Xem thống kê processes
├── setpri.c        # Set priority thủ công
├── mlfqmon.c       # Legacy monitor (plain text)
├── demo.c          # Tạo demo workload
├── cpu_bound.c     # Test CPU-intensive
├── io_bound.c      # Test I/O-intensive
└── schedtest.c     # Test tổng hợp
```

### Memory Safety Notes ⚠️
**Critical**: xv6 kernel và user stacks chỉ có 4KB
- ❌ **Stack allocation**: Không dùng `struct pstat ps;` (stack overflow)
- ✅ **Heap allocation**: 
  - Kernel: `ps = kalloc(); ... kfree(ps);`
  - User: `ps = malloc(sizeof(struct pstat)); ... free(ps);`

---

## MLFQ Rules (theo OSTEP)

1. **Rule 1**: Nếu Priority(A) > Priority(B), A chạy trước
2. **Rule 2**: Nếu Priority(A) = Priority(B), chạy Round-Robin
3. **Rule 3**: Process mới bắt đầu ở Queue cao nhất (Queue 0)
4. **Rule 4a**: Nếu dùng hết time slice → demote xuống queue thấp hơn
5. **Rule 4b**: Nếu nhường CPU trước (sleep/I/O) → giữ nguyên queue
6. **Rule 5**: Priority Boost định kỳ để chống starvation

---

## Troubleshooting

### ⚠️ Stack Overflow Issues
**Problem**: `exec <program> failed` trong xv6

**Cause**: Large structs (>1KB) allocated on 4KB stack

**Solution**:
```c
// ❌ WRONG - Stack allocation
struct pstat ps;
getpstat(&ps);

// ✅ CORRECT - Heap allocation
struct pstat *ps = malloc(sizeof(struct pstat));
if(!ps) return -1;
getpstat(ps);
// ... use ps ...
free(ps);
```

### Lỗi "Couldn't find riscv64 GCC"
```bash
# Trong WSL Ubuntu
sudo apt update
sudo apt install gcc-riscv64-unknown-elf

# Nếu package không tồn tại, dùng alternative
sudo apt install gcc-riscv64-linux-gnu
make TOOLPREFIX=riscv64-linux-gnu- qemu
```

### Build errors
```bash
# Clean rebuild
make clean
make qemu

# Nếu vẫn lỗi, check toolchain
riscv64-linux-gnu-gcc --version
```

### QEMU không thoát được
- **Method 1**: `Ctrl+A` sau đó nhấn `X`
- **Method 2**: Từ terminal khác: `pkill qemu`

### ANSI colors không hiển thị
Monitor tự động detect và fallback sang plain text mode. Nếu muốn force ANSI:
```bash
# Check TERM variable trong xv6
$ echo $TERM

# Nếu cần, set trong shell startup
export TERM=xterm-256color
```

### Automated tests fail
```bash
# 1. Verify kernel has getpstat syscall
$ simple_test

# 2. Check if stress test works
$ stress 1 1 0 5

# 3. Run individual tests
$ mlfq_test   # Xem test nào fail
```

### Monitor lag/performance issues
```bash
# Tăng refresh interval để giảm overhead
$ monitor 20    # Refresh mỗi 20 ticks thay vì 10

# Hoặc giảm số processes
$ stress 1 1 0 15   # Ít workers hơn
```

---

## Tham khảo

- [xv6 Book](https://pdos.csail.mit.edu/6.828/2023/xv6/book-riscv-rev3.pdf)
- [OSTEP - MLFQ Chapter](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched-mlfq.pdf)
