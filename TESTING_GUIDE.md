# MLFQ Scheduler Testing Guide

## Table of Contents
1. [Quick Start](#quick-start)
2. [Test Tools Overview](#test-tools-overview)
3. [Manual Testing](#manual-testing)
4. [Automated Testing](#automated-testing)
5. [Performance Benchmarking](#performance-benchmarking)
6. [Troubleshooting Tests](#troubleshooting-tests)
7. [Expected Results](#expected-results)

---

## Quick Start

### Validate Build (Host Machine)
```bash
# On Windows with WSL
cd "xv6-mlfq-scheduler"
./quick_test.sh

# Expected output:
# ✓ Build successful
# ✓ All binaries present
# ✓ Ready to test in xv6
```

### Quick Verification (Inside xv6)
```bash
# Start xv6
make qemu

# Inside xv6, run tests in order:
$ simple_test        # Step 1: Verify syscall works
$ mlfq_test          # Step 2: Automated verification (1 min)
$ monitor 5 &        # Step 3: Visual demo
$ stress 2 2 1 20    # Step 4: Workload demo
```

---

## Test Tools Overview

### 1. simple_test
**Purpose**: Minimal syscall verification  
**Runtime**: < 1 second  
**Use Case**: Quick sanity check

```bash
$ simple_test
```

**Expected Output**:
```
Testing getpstat syscall...
✓ Syscall succeeded
✓ Global ticks: 123
✓ Found 3 processes
✓ Basic validation PASSED
```

### 2. mlfq_test
**Purpose**: Comprehensive MLFQ behavior verification  
**Runtime**: ~60 seconds  
**Use Case**: Validate scheduler correctness

```bash
$ mlfq_test
```

**Test Coverage**:
- ✓ Syscall functionality (getpstat returns valid data)
- ✓ Priority demotion (CPU-bound → lower queues)
- ✓ Priority preservation (I/O-bound stays high)
- ✓ Fairness (I/O finishes before CPU)
- ✓ Queue distribution (processes spread across queues)

### 3. monitor
**Purpose**: Real-time TUI visualization  
**Runtime**: Continuous (manual exit)  
**Use Case**: Visual demo and debugging

```bash
$ monitor [refresh_ticks]

# Examples:
$ monitor           # Default: 10 ticks refresh
$ monitor 5         # Fast: 5 ticks refresh
$ monitor 20        # Slow: 20 ticks refresh
```

**Features**:
- ANSI colors (auto-fallback for non-ANSI)
- Queue visualization with progress bars
- Process table (top 15 processes)
- System stats (ticks, boost info)

### 4. stress
**Purpose**: Workload generator for testing  
**Runtime**: User-specified  
**Use Case**: Create diverse workloads

```bash
$ stress [cpu_workers] [io_workers] [mixed_workers] [duration_sec]

# Examples:
$ stress            # Default: 2 CPU, 2 I/O, 1 mixed, 20s
$ stress 5 1 0 30   # CPU-heavy: 5 CPU, 1 I/O, 30s
$ stress 1 5 0 30   # I/O-heavy: 1 CPU, 5 I/O, 30s
$ stress 2 2 2 60   # Balanced: 2 each type, 60s
```

### 5. Legacy Tools
```bash
$ pstat             # Text-based process stats
$ mlfqmon 5 50      # Legacy monitor (plain text)
$ schedtest         # CPU vs I/O comparison
$ cpu_bound 10      # Single CPU-bound process
$ io_bound 10 5     # Single I/O-bound process
```

---

## Manual Testing

### Test Case 1: Priority Demotion (CPU-bound)
**Objective**: Verify CPU-intensive processes demote to lower queues

**Steps**:
```bash
# 1. Start monitor
$ monitor 5 &

# 2. Launch CPU-bound process
$ cpu_bound 20 &

# 3. Observe in monitor:
#    - Process starts at Queue 0 (HIGH)
#    - After ~1-2 ticks, moves to Queue 1 (MEDIUM)
#    - After ~2-4 more ticks, moves to Queue 2 (LOW)
#    - DEMOTE count increases
```

**Expected Result**:
- Initial priority: 0
- Final priority: 2
- DEMOTE count: ≥ 2
- Process completes slower than I/O-bound

### Test Case 2: Priority Preservation (I/O-bound)
**Objective**: Verify I/O processes stay in high queue

**Steps**:
```bash
# 1. Start monitor
$ monitor 5 &

# 2. Launch I/O-bound process
$ io_bound 15 3 &

# 3. Observe in monitor:
#    - Process stays at Queue 0 (HIGH)
#    - Frequently shows SLEEP state
#    - DEMOTE count remains 0
```

**Expected Result**:
- Priority: Always 0
- DEMOTE count: 0
- Process completes quickly

### Test Case 3: Priority Boost
**Objective**: Verify all processes return to Q0 every 100 ticks

**Steps**:
```bash
# 1. Start monitor
$ monitor 5 &

# 2. Create mixed workload
$ stress 3 2 0 40 &

# 3. Watch "Next Boost in:" counter in monitor
# 4. When boost happens:
#    - All processes move to Q0
#    - Queue bars show: Q0=max, Q1=0, Q2=0
#    - "Last Boost" timestamp updates
```

**Expected Result**:
- Boost occurs every 100 ticks
- All runnable processes briefly in Q0
- CPU-bound processes quickly demote again

### Test Case 4: Mixed Fairness
**Objective**: I/O-bound finishes before CPU-bound

**Steps**:
```bash
# Launch both types simultaneously
$ schedtest

# Or manually:
$ cpu_bound 20 &
$ io_bound 15 3 &

# Monitor completion order
```

**Expected Result**:
```
I/O-bound process finished first!
CPU-bound process finished second.
```

### Test Case 5: Queue Distribution
**Objective**: Verify processes spread across queues

**Steps**:
```bash
# 1. Start monitor
$ monitor 10 &

# 2. Heavy CPU workload
$ stress 6 0 0 30 &

# 3. Observe queue bars:
#    - Q0: Few processes (newly spawned or boosted)
#    - Q1: Some processes (recently demoted)
#    - Q2: Most processes (CPU-heavy steady state)
```

**Expected Result**:
- At least 2 queues have processes
- Q2 has majority of CPU-bound workers
- Distribution shows: Q0 < Q1 ≤ Q2

---

## Automated Testing

### Running Full Test Suite
```bash
$ mlfq_test
```

### Understanding Test Output

#### Test 1: Syscall Functionality
```
============================================================
  Test: Syscall Functionality
============================================================
  [✓ PASS] getpstat() returns 0
  [✓ PASS] global_ticks > 0
  [✓ PASS] total_processes > 0
  [✓ PASS] found processes >= 2
```

**What it tests**:
- getpstat syscall returns success (0)
- System stats are populated
- At least init and sh are running

#### Test 2: Priority Demotion
```
============================================================
  Test: Priority Demotion (CPU-bound)
============================================================
  [✓ PASS] initial priority = 0
  [✓ PASS] priority decreased
  [✓ PASS] demote count > 0
  Details: Priority 0 -> 1 -> 2, demoted 2 times
```

**What it tests**:
- Forks CPU-intensive child
- Monitors priority over time
- Verifies demotion happens (0 → 1 → 2)
- Checks demote count increments

#### Test 3: Priority Preservation
```
============================================================
  Test: Priority Preservation (I/O-bound)
============================================================
  [✓ PASS] initial priority = 0
  [✓ PASS] priority stayed at 0
  [✓ PASS] demote count = 0
  Details: Priority 0 -> 0 -> 0, demoted 0 times
```

**What it tests**:
- Forks I/O-bound child (frequent sleep)
- Monitors priority over time
- Verifies priority stays at 0
- Checks demote count is 0

#### Test 4: Mixed Fairness
```
============================================================
  Test: Mixed Workload Fairness
============================================================
  First finished: PID 5 at 42 ticks
  Second finished: PID 4 at 156 ticks
  [✓ PASS] I/O finished before CPU
```

**What it tests**:
- Forks both CPU-bound and I/O-bound
- Tracks completion order
- Verifies I/O finishes first (responsiveness)

#### Test 5: Queue Distribution
```
============================================================
  Test: Queue Distribution
============================================================
  Queue distribution:
    Q0: 1 processes
    Q1: 1 processes
    Q2: 3 processes
  [✓ PASS] processes distributed
  [✓ PASS] at least 2 queues used
```

**What it tests**:
- Spawns multiple CPU-bound workers
- Lets them distribute across queues
- Verifies spread (not all in one queue)

### Test Summary Interpretation
```
╔══════════════════════════════════════════════════════════════╗
║                      TEST SUMMARY                            ║
╠══════════════════════════════════════════════════════════════╣
║  Total Tests:   15                                           ║
║  Passed:        15  ✓                                        ║
║  Failed:         0  ✗                                        ║
║  Result:        ALL TESTS PASSED! 🎉                        ║
╚══════════════════════════════════════════════════════════════╝
```

**Exit Codes**:
- `0`: All tests passed
- `1`: At least one test failed

---

## Performance Benchmarking

### Benchmark 1: Turnaround Time (CPU-bound)
**Measures**: Time to complete CPU-intensive work

```bash
# Test command
$ cpu_bound 20

# Measure completion time
# Record: start tick → end tick
```

**Expected**:
- Completion time: 80-120 ticks (varies with load)
- Priority at end: 2 (LOW)
- Demote count: ≥ 2

### Benchmark 2: Response Time (I/O-bound)
**Measures**: Time between I/O operations

```bash
# Test command
$ io_bound 10 5

# Observe sleep → wake latency
```

**Expected**:
- Each wake latency: < 10 ticks
- Priority: Always 0 (HIGH)
- Quick completion (~50-70 ticks)

### Benchmark 3: CPU Utilization
**Measures**: Scheduler overhead

```bash
# 1. Start monitor
$ monitor 10 &

# 2. Heavy workload
$ stress 8 0 0 60 &

# 3. Observe:
#    - % of time with running process
#    - Scheduler responsiveness
```

**Expected**:
- High CPU utilization (most ticks have running proc)
- Low idle time
- Smooth queue transitions

### Benchmark 4: Starvation Prevention
**Measures**: Low priority proc gets CPU eventually

```bash
# 1. Monitor
$ monitor 5 &

# 2. Heavy CPU load
$ stress 5 0 0 120 &

# 3. Watch for boosts (every 100 ticks)
#    - All procs return to Q0
#    - Low priority procs get chance to run
```

**Expected**:
- Boost every 100 ticks
- No process starves indefinitely
- Q2 processes eventually scheduled

---

## Troubleshooting Tests

### Problem: simple_test fails
**Symptoms**: "getpstat failed" or kernel panic

**Solutions**:
```bash
# 1. Verify kernel has syscall
$ grep getpstat kernel/syscall.c

# 2. Check syscall number
$ grep SYS_getpstat kernel/syscall.h

# 3. Rebuild clean
$ make clean
$ make qemu
```

### Problem: mlfq_test hangs
**Symptoms**: Test stuck, no output

**Solutions**:
- **Ctrl+C** to kill
- Check if test created zombie children
- Reboot xv6 (Ctrl+A X, then make qemu)

### Problem: Tests fail intermittently
**Symptoms**: Sometimes pass, sometimes fail

**Causes**:
- Timing-dependent tests
- Insufficient test duration
- Race conditions

**Solutions**:
```c
// In mlfq_test.c, increase sleep times:
sleep(20);  // Instead of sleep(10)
sleep(30);  // Give more time for demotion
```

### Problem: Monitor shows wrong data
**Symptoms**: Counts don't match, strange values

**Debugging**:
```bash
# 1. Cross-check with pstat
$ pstat

# 2. Check kernel tracking
# Add printfs in kernel/proc.c:
printf("scheduler: pid %d, prio %d\n", p->pid, p->priority);

# 3. Verify malloc succeeded
# In monitor.c, check malloc return
```

### Problem: Stress test creates too many processes
**Symptoms**: "fork failed", slow performance

**Solution**:
```bash
# Reduce worker counts
$ stress 2 2 1 20   # Instead of stress 8 8 8 120

# Or increase NPROC in kernel/param.h:
#define NPROC 128   // Instead of 64
```

---

## Expected Results

### Summary Table

| Test | Duration | Expected Outcome |
|------|----------|------------------|
| simple_test | < 1s | Syscall succeeds, data valid |
| mlfq_test | ~60s | All 15 tests PASS |
| CPU demotion | ~40s | Priority 0→2, demote≥2 |
| I/O preservation | ~50s | Priority=0, demote=0 |
| Mixed fairness | ~30s | I/O finishes first |
| Priority boost | ~110s | Boost at tick 100, 200, ... |
| Queue distribution | ~30s | ≥2 queues used |

### Visual Demo Checklist

✅ **Before Demo**:
- [ ] Build completes with 0 errors
- [ ] simple_test passes
- [ ] mlfq_test ALL PASSED

✅ **During Demo**:
- [ ] Monitor shows ANSI colors (or fallback)
- [ ] CPU-bound processes demote to Q2
- [ ] I/O-bound processes stay at Q0
- [ ] Priority boost visible every 100 ticks
- [ ] Queue bars update in real-time

✅ **After Demo**:
- [ ] No zombie processes left
- [ ] System still responsive (can run new commands)

---

## Advanced Testing

### Load Testing
```bash
# Maximum load
$ stress 10 10 10 60 &

# Monitor stability
$ monitor 10

# Expected: No crashes, scheduler still fair
```

### Edge Cases
```bash
# 1. Single process
$ stress 1 0 0 30
# Expected: Stays in high queue (no competition)

# 2. Boost during heavy load
$ stress 8 0 0 150
# Watch boosts at ticks 100, 200

# 3. Rapid fork/exit
$ forktest
# Expected: No deadlocks, clean termination
```

### Stress Test Scenarios

#### Scenario A: CPU-Heavy
```bash
$ stress 8 1 0 60
```
Expected: Most processes in Q2

#### Scenario B: I/O-Heavy
```bash
$ stress 1 8 0 60
```
Expected: Most processes in Q0

#### Scenario C: Balanced
```bash
$ stress 3 3 3 60
```
Expected: Even distribution across queues

---

## Conclusion

This testing guide provides comprehensive coverage of MLFQ scheduler verification. Start with automated tests (`mlfq_test`), then use manual testing with `monitor` and `stress` for visual confirmation and demos.

**Quick Demo Sequence** (for presentation):
```bash
make qemu
$ mlfq_test              # 1 min - prove correctness
$ monitor 5 &            # Start visualization
$ stress 3 3 1 30        # 30 sec - visual demo
# Exit: Ctrl+C monitor, then Ctrl+A X
```

For issues, refer to [Troubleshooting Tests](#troubleshooting-tests) or main [MLFQ_README.md](MLFQ_README.md).
