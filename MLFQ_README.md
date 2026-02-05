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

### Xem thông tin scheduler
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

### Monitor real-time
```bash
$ mlfqmon [interval] [iterations]

# Ví dụ: refresh mỗi 5 ticks, chạy 20 lần
$ mlfqmon 5 20
```

### Set priority thủ công
```bash
$ setpri <pid> <priority>

# Ví dụ: Set PID 3 xuống priority 2 (LOW)
$ setpri 3 2
```

---

## Demo đầy đủ

### Demo 1: So sánh CPU-bound vs I/O-bound

Mở xv6 và chạy:
```bash
$ schedtest
```

Kết quả mong đợi:
- **I/O-bound processes** hoàn thành nhanh hơn (được ưu tiên)
- **CPU-bound processes** bị demote và chạy sau

### Demo 2: Monitor trực quan

```bash
# Bước 1: Chạy monitor trong background
$ mlfqmon 3 50 &

# Bước 2: Chạy demo workload
$ demo

# Bước 3: Quan sát sự thay đổi priority trên màn hình
```

### Demo 3: Test riêng từng loại process

```bash
# CPU-bound (sẽ bị demote)
$ cpu_bound 10

# I/O-bound (sẽ giữ priority cao)
$ io_bound 10 5
```

---

## Giải thích output mlfqmon

```
============================================================
        MLFQ SCHEDULER MONITOR (Refresh #5)
============================================================
Time: 234 ticks

QUEUE STATUS:
------------------------------------------------------------
Queue 0 [HIGH  ] (2) [####                ]   ← 2 processes ở HIGH
Queue 1 [MEDIUM] (1) [==                  ]   ← 1 process ở MEDIUM
Queue 2 [LOW   ] (2) [----                ]   ← 2 processes ở LOW
------------------------------------------------------------

PROCESS TABLE:
PID     PRIO    STATE   TICKS   TOTAL   NAME
------------------------------------------------------------
1       0       SLEEP   0       5       init      ← I/O-bound, giữ HIGH
2       0       SLEEP   0       3       sh        ← I/O-bound, giữ HIGH
*4      2       RUN     3       15      cpu_work* ← CPU-bound, bị demote!
5       0       SLEEP   0       2       io_work   ← I/O-bound, giữ HIGH
------------------------------------------------------------
Total: 4 | Running: 1 | Sleeping: 3
============================================================
```

Ký hiệu:
- `*...*`: Process đang RUN
- `PRIO 0`: Highest priority
- `PRIO 2`: Lowest priority
- `TICKS`: Ticks đã dùng trong time slice hiện tại
- `TOTAL`: Tổng ticks đã sử dụng

---

## Cấu trúc source code

```
kernel/
├── param.h         # MLFQ constants (NMLFQ, time slices, boost interval)
├── proc.h          # Thêm fields: priority, ticks_used, ticks_total
├── proc.c          # MLFQ scheduler, priority_boost(), getprocinfo()
├── sysproc.c       # Syscalls: sys_getpinfo, sys_setpriority
├── syscall.h/c     # Syscall numbers và registration
└── defs.h          # Function declarations

user/
├── pstat.c         # Xem thống kê processes
├── setpri.c        # Set priority thủ công
├── mlfqmon.c       # Real-time monitor
├── demo.c          # Tạo demo workload
├── cpu_bound.c     # Test CPU-intensive
├── io_bound.c      # Test I/O-intensive
└── schedtest.c     # Test tổng hợp
```

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

### Lỗi "Couldn't find riscv64 GCC"
```bash
# Cài toolchain
sudo apt install gcc-riscv64-unknown-elf

# Hoặc dùng alternative prefix
make TOOLPREFIX=riscv64-linux-gnu- qemu
```

### Lỗi build
```bash
make clean
make qemu
```

### QEMU không thoát được
Nhấn `Ctrl+A` sau đó nhấn `X`

---

## Tham khảo

- [xv6 Book](https://pdos.csail.mit.edu/6.828/2023/xv6/book-riscv-rev3.pdf)
- [OSTEP - MLFQ Chapter](https://pages.cs.wisc.edu/~remzi/OSTEP/cpu-sched-mlfq.pdf)
