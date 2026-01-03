#!/bin/bash
# System tuning for 100% hardware bandwidth

echo "Tuning system for maximum memory bandwidth..."

# 1. CPU governor to performance
echo "Setting CPU governor to performance..."
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo performance | sudo tee $cpu > /dev/null 2>&1
done

# 2. Disable CPU frequency scaling
echo "Disabling CPU frequency scaling..."
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo > /dev/null 2>&1
echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost > /dev/null 2>&1

# 3. Enable transparent huge pages
echo "Enabling transparent huge pages..."
echo always | sudo tee /sys/kernel/mm/transparent_hugepage/enabled > /dev/null 2>&1
echo always | sudo tee /sys/kernel/mm/transparent_hugepage/defrag > /dev/null 2>&1

# 4. Increase vm limits
echo "Tuning VM parameters..."
sudo sysctl -w vm.nr_hugepages=1024 > /dev/null 2>&1
sudo sysctl -w vm.swappiness=0 > /dev/null 2>&1
sudo sysctl -w vm.dirty_ratio=80 > /dev/null 2>&1
sudo sysctl -w vm.dirty_background_ratio=5 > /dev/null 2>&1

# 5. Drop caches
echo "Dropping caches..."
sync
echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null 2>&1

echo "System tuned! Run benchmark now."
echo ""
echo "To run benchmark pinned to cores 0-7 (physical cores only):"
echo "taskset -c 0-7 ./insane_bench"
