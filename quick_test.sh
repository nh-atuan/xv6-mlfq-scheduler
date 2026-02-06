#!/bin/bash
# quick_test.sh - Quick validation script for MLFQ implementation
# Run this OUTSIDE xv6 to build and prepare test

set -e

echo "=========================================="
echo "  MLFQ Quick Validation Script"
echo "=========================================="
echo ""

# Navigate to project directory
cd "$(dirname "$0")"

echo "[1/3] Building xv6..."
make clean > /dev/null 2>&1
if make fs.img 2>&1 | grep -i "error"; then
    echo "❌ Build FAILED"
    exit 1
fi
echo "✅ Build successful"
echo ""

echo "[2/3] Checking binaries..."
MISSING=""
for prog in test_pstat simple_test monitor stress; do
    if [ ! -f "user/_${prog}" ]; then
        MISSING="$MISSING $_${prog}"
    fi
done

if [ -n "$MISSING" ]; then
    echo "❌ Missing binaries:$MISSING"
    exit 1
fi
echo "✅ All binaries present"
echo ""

echo "[3/3] Ready to test in QEMU"
echo ""
echo "=========================================="
echo "  Next Steps:"
echo "=========================================="
echo ""
echo "Run: make qemu"
echo ""
echo "Then in xv6 shell, try these commands:"
echo ""
echo "  Basic tests:"
echo "    $ simple_test      # Test syscall works"
echo "    $ test_pstat       # Test full pstat data"
echo "    $ pstat            # Test legacy getpinfo"
echo ""
echo "  Monitor demo:"
echo "    $ monitor 5 20     # Watch for 20 refreshes"
echo ""
echo "  Stress test (in another QEMU window):"
echo "    $ stress 2 2 150   # 2 CPU + 2 I/O workers"
echo ""
echo "=========================================="
