# ⚙️ Hardware Tuning & Register Reference

This document details every kernel sysfs interface, hardware register, and driver mechanism manipulated by ThinkPower.

---

## 1. Processor & Silicon (AMD Renoir Zen 2 / 4750U)

### 1.1 CPU Turbo Boost
- **Sysfs Node**: `/sys/devices/system/cpu/cpufreq/boost`
- **Default (On AC / Performance)**: `1`
- **Ultra / Smart Save**: `0`
- **Mechanism**: AMD Zen 2 processors scale voltage non-linearly above base clock (1.7GHz). When boost is enabled, single-core bursts to 4.1GHz push core voltage up to 1.35V+, causing short package spikes of 25W–40W. Setting `boost=0` limits the core ceiling to 1.7GHz at ~0.8V, cutting transient thermal and power spikes by more than 60%.

### 1.2 Scaling Governor
- **Sysfs Node**: `/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`
- **Performance / Balanced / Smart Save**: `schedutil`
- **Ultra Save**: `powersave`
- **Mechanism**: Under `acpi-cpufreq`, `powersave` pins all active cores to the minimum hardware P-state (1.4GHz on 4750U), preventing any upward frequency transition and maintaining minimal operating voltage.

### 1.3 SMT (Simultaneous Multithreading / Hyperthreading)
- **Sysfs Node**: `/sys/devices/system/cpu/smt/control`
- **Performance / Balanced / Smart Save**: `on` (8 Cores / 16 Threads)
- **Ultra Save**: `off` (8 Physical Cores / 8 Threads)
- **Mechanism**: Toggling SMT off offlines sibling logical threads (CPUs 8–15). This eliminates thread scheduling interrupts, L1/L2 cache thrashing, and reduces idle leakage current across the CCX while retaining all 8 full physical Zen 2 execution cores.

### 1.4 APU Package Power Capping (`ryzenadj` - Optional)
- **Tool**: `ryzenadj` (AUR: `ryzenadj` or CachyOS repos)
- **Ultra Setting**: `--stapm-limit=6000 --fast-limit=8000 --slow-limit=6000 --tctl-temp=65`
- **Restore Setting**: `--stapm-limit=25000 --fast-limit=30000 --slow-limit=25000`
- **Mechanism**: Hard caps the Sustained Power Tracking Limit (STAPM) to 6.0W in hardware.

---

## 2. Graphics & Display Panel

### 2.1 AMD Adaptive Backlight Management (ABM)
- **Sysfs Node**: `/sys/class/drm/card1-eDP-1/amdgpu/panel_power_savings`
- **Levels**:
  - `0`: Off (Performance)
  - `1`: Subtle backlight reduction (Balanced)
  - `2`: Moderate backlight reduction, color preserved (Smart Save)
  - `4`: Maximum backlight power reduction (Ultra Save)
- **Mechanism**: ABM uses a dedicated hardware pixel luminance boost algorithm in the AMD display engine, dimming the physical LED backlight while compensating pixel RGB values, reducing display power by up to 1.0W without perceived brightness drop.

### 2.2 Display Refresh Rate Downclocking (48Hz)
- **Command**: `kscreen-doctor output.1.mode.2` (48.04Hz) / `output.1.mode.1` (60.06Hz)
- **Mechanism**: The ThinkPad L15 Gen 1 eDP panel supports a native 48Hz mode. Reducing scanout frequency from 60Hz to 48Hz reduces display controller PHY transmission clock cycles, saving ~0.5W–0.8W.

### 2.3 AMDGPU Dynamic Power Management (DPM)
- **Sysfs Node**: `/sys/class/drm/card1/device/power_dpm_force_performance_level`
- **Ultra Save**: `low`
- **Balanced / Performance**: `auto`

---

## 3. Bus Links & Storage I/O

### 3.1 PCIe Active State Power Management (ASPM)
- **Sysfs Node**: `/sys/module/pcie_aspm/parameters/policy`
- **Default**: `default`
- **Ultra Save**: `powersupersave`
- **Mechanism**: Forces all PCIe root ports and downstream endpoint devices (Samsung PM9A1 NVMe SSD, Intel AX200 Wi-Fi, Realtek GbE) into ASPM L1.1 and L1.2 low-power sub-states whenever links are idle.

### 3.2 Linux Kernel Laptop Mode & Dirty Page Buffering
- **Sysfs Nodes**:
  - `/proc/sys/vm/laptop_mode` (Set to `5` in Ultra, `0` in default)
  - `/proc/sys/vm/dirty_writeback_centisecs` (Set to `6000` [60s] in Ultra, `500` [5s] in default)
- **Mechanism**: By deferring dirty filesystem page flushes to 60-second batches, the NVMe SSD controller is allowed to stay in deep Autonomous Power State Transition (APST) sleep states for minutes continuously.

---

## 4. Chassis & Peripheral Power

### 4.1 ThinkPad Embedded Controller (EC) Platform Profile
- **Sysfs Node**: `/sys/firmware/acpi/platform_profile`
- **Choices**: `low-power`, `balanced`, `performance`
- **Ultra Save**: `low-power` (Embedded Controller quiets or completely turns off the cooling fan to 0 RPM).

### 4.2 Keyboard Backlight LED (with State Memory)
- **Sysfs Node**: `/sys/class/leds/tpacpi::kbd_backlight/brightness`
- **Ultra Save**: `0` (Off)
- **Smart Save / Balanced / Performance**: Automatically restored to previous brightness level (`1` or `2`) from `~/.cache/prev_kbd_backlight`.
- **Power Savings**: ~0.4W.

### 4.3 Audio Codec Powerdown
- **Sysfs Node**: `/sys/module/snd_hda_intel/parameters/power_save`
- **Ultra Save**: `1` (1 second silence timeout before codec D3 powerdown)
- **Balanced / Performance**: `10` (10 seconds timeout)

### 4.4 Background Search Indexer (KDE Baloo)
- **Command**: `balooctl6 suspend` / `balooctl6 resume`
- **Mechanism**: Suspends filesystem index database operations, preventing CPU wakeups and disk read activity during emergency battery conservation.
