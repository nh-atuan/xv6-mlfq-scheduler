# MLFQ Scheduler cho xv6 (RISC-V)

## Tổng quan

Dự án này mở rộng hệ điều hành xv6 (RISC-V) bằng cách thay thế bộ lập lịch Round-Robin mặc định bằng bộ lập lịch **Multi-Level Feedback Queue (MLFQ)**. MLFQ là thuật toán lập lịch được sử dụng rộng rãi trong các hệ điều hành hiện đại, cho phép phân biệt và xử lý hiệu quả cả tiến trình CPU-bound lẫn I/O-bound.

**Đặc điểm chính:**
- 3 hàng đợi ưu tiên (Queue 0: cao nhất, Queue 2: thấp nhất)
- Time quantum tăng dần theo mức ưu tiên (1, 2, 4 ticks)
- Cơ chế feedback tự động: hạ ưu tiên khi dùng hết quantum, giữ/tăng ưu tiên khi yield sớm
- Priority boost định kỳ (mỗi 100 ticks) để chống starvation
- 2 system call mới: `getpinfo()` và `setpriority()`
- Các chương trình test và visualization (terminal-based monitor)

---

## Thông tin nhóm

| Họ và tên | MSSV |
|-----------|------|
| (Điền thông tin thành viên 1) | (MSSV) |
| (Điền thông tin thành viên 2) | (MSSV) |
| (Điền thông tin thành viên 3) | (MSSV) |

**Môn học:** Hệ Điều Hành
**Trường:** Đại học Khoa học Tự nhiên - ĐHQG TP.HCM (fit@hcmus)

---

## Cấu trúc dự án

### Các file kernel đã chỉnh sửa

| File | Mô tả |
|------|-------|
| `kernel/param.h` | Thêm các hằng số MLFQ: `NMLFQ=3`, `MLFQ_TICKS_0=1`, `MLFQ_TICKS_1=2`, `MLFQ_TICKS_2=4`, `BOOST_INTERVAL=100` |
| `kernel/proc.h` | Mở rộng `struct proc` với các trường: `priority`, `ticks_used`, `ticks_total`, `last_run_time` |
| `kernel/proc.c` | Viết lại `scheduler()` cho MLFQ, thêm `priority_boost()`, `get_time_slice()`, cập nhật `yield()`, `sleep()`, `wakeup()`, thêm `getprocinfo()`, `setprocpriority()` |
| `kernel/trap.c` | Xử lý timer interrupt để gọi `yield()` - đếm tick và điều chỉnh priority |
| `kernel/syscall.h` | Thêm `SYS_getpinfo` (22) và `SYS_setpriority` (23) |
| `kernel/syscall.c` | Đăng ký 2 syscall mới vào bảng syscall |
| `kernel/sysproc.c` | Thêm `sys_getpinfo()` và `sys_setpriority()` |
| `kernel/defs.h` | Khai báo prototype cho `getprocinfo()`, `setprocpriority()` |

### Các file user-space mới

| File | Mô tả |
|------|-------|
| `user/user.h` | Thêm prototype `getpinfo()` và `setpriority()` |
| `user/usys.pl` | Thêm entry cho 2 syscall mới |
| `user/cpu_bound.c` | Chương trình test CPU-bound - thực hiện tính toán nặng, sẽ bị hạ ưu tiên |
| `user/io_bound.c` | Chương trình test I/O-bound - sleep thường xuyên, sẽ giữ ưu tiên cao |
| `user/schedtest.c` | Test tổng hợp: tạo đồng thời CPU-bound và I/O-bound processes |
| `user/pstat.c` | Hiển thị thống kê scheduler (bảng tiến trình với priority, ticks) |
| `user/setpri.c` | Đặt priority thủ công cho process: `setpri <pid> <priority>` |
| `user/mlfqmon.c` | Monitor real-time: hiển thị trạng thái hàng đợi MLFQ liên tục |
| `user/demo.c` | Chương trình demo tạo mixed workload để quan sát MLFQ |

### Nhánh phát triển (Branches)

| Branch | Mô tả |
|--------|-------|
| `main` | Code hoàn chỉnh với MLFQ scheduler |
| `feature/mlfq-core` | PR #1: Cài đặt MLFQ core scheduler |
| `feature/syscalls` | PR #3: Thêm system calls (getpinfo, setpriority) |
| `feature/test-programs` | PR #2: Các chương trình test |
| `feature/visualization` | PR #4: Terminal-based visualization (mlfqmon) |
| `feature/web-UI` | (Đang phát triển) TUI monitor với ANSI colors + các test bổ sung |

---

## Hướng dẫn chạy

### Yêu cầu hệ thống

- **QEMU** (RISC-V): `qemu-system-riscv64`
- **RISC-V GCC Toolchain**: `riscv64-unknown-elf-gcc` hoặc `riscv64-linux-gnu-gcc`
- **Make**, **Git**

### Bước 1: Build và chạy xv6

```bash
# Clone repository
git clone <repo-url>
cd xv6-mlfq-scheduler

# Build và chạy
make clean
make qemu
```

### Bước 2: Chạy các chương trình test trong xv6 shell

```bash
# Test CPU-bound (sẽ bị demote xuống queue thấp)
$ cpu_bound

# Test I/O-bound (sẽ giữ nguyên ở queue cao)
$ io_bound

# Test tổng hợp (CPU + I/O cùng lúc)
$ schedtest

# Xem thống kê scheduler
$ pstat

# Monitor real-time (cập nhật mỗi 10 ticks, 50 lần)
$ mlfqmon 10 50

# Đặt priority thủ công
$ setpri <pid> <priority>
# Ví dụ: setpri 5 0  (đặt PID 5 lên priority cao nhất)

# Demo (chạy kết hợp với mlfqmon)
# Terminal 1: demo
# Terminal 2: mlfqmon
```

### Bước 3: Quan sát hành vi MLFQ

Kịch bản demo điển hình:
1. Chạy `mlfqmon 5 100 &` (monitor chạy nền)
2. Chạy `demo` hoặc `schedtest`
3. Quan sát:
   - Tiến trình CPU-bound bị hạ từ Queue 0 xuống Queue 1, rồi Queue 2
   - Tiến trình I/O-bound giữ nguyên ở Queue 0
   - Priority boost định kỳ đưa tất cả về Queue 0

### Thoát QEMU

Nhấn `Ctrl+A` rồi `X` để thoát QEMU.

---

## Thiết kế MLFQ

### Các quy tắc (Rules)

1. **Rule 1:** Tiến trình ở queue có ưu tiên cao hơn chạy trước
2. **Rule 2:** Cùng ưu tiên -> Round-Robin
3. **Rule 3:** Dùng hết time quantum -> hạ xuống queue thấp hơn
4. **Rule 4:** Yield sớm (I/O) -> giữ/tăng ưu tiên
5. **Rule 5:** Priority boost định kỳ (mỗi 100 ticks) -> chống starvation

### Cấu hình

| Tham số | Giá trị | Mô tả |
|---------|---------|-------|
| `NMLFQ` | 3 | Số lượng hàng đợi ưu tiên |
| `MLFQ_TICKS_0` | 1 | Time quantum Queue 0 (cao nhất) |
| `MLFQ_TICKS_1` | 2 | Time quantum Queue 1 (trung bình) |
| `MLFQ_TICKS_2` | 4 | Time quantum Queue 2 (thấp nhất) |
| `BOOST_INTERVAL` | 100 | Chu kỳ priority boost (ticks) |

---

## Link video demo

*(Thêm link video demo tại đây)*

---

## Tài liệu tham khảo

- Remzi H. Arpaci-Dusseau, Andrea C. Arpaci-Dusseau. *Operating Systems: Three Easy Pieces (OSTEP)* - Chapter 8: Multi-Level Feedback Queue.
- MIT 6.S081: Operating System Engineering - xv6 lab.
- xv6: a simple, Unix-like teaching operating system (RISC-V version).
