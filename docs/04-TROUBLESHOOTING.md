# 🔧 Troubleshooting & FAQ

---

## 1. Permission Denied / Sudo Prompt when Switching Profiles

### Symptom
Clicking a power profile in the tray prompts for a password, or the hardware state (frequency, boost, keyboard light) does not change.

### Solution
Ensure the passwordless sudo rule is installed:
```bash
sudo cp src/config/99-power-profile-manager /etc/sudoers.d/99-power-profile-manager
sudo chmod 0440 /etc/sudoers.d/99-power-profile-manager
```

Verify with:
```bash
sudo -n /usr/local/bin/power-profile-manager status
```
If this exits with code 0 without asking for a password, permissions are configured properly.

---

## 2. Dependencies Checklist

### Arch Linux / CachyOS / Manjaro
```bash
sudo pacman -S python-gobject libayatana-appindicator power-profiles-daemon upower libnotify
```

### Fedora / RHEL
```bash
sudo dnf install python3-gobject libayatana-appindicator power-profiles-daemon upower libnotify
```

### Debian / Ubuntu
```bash
sudo apt install python3-gi gir1.2-ayatanaappindicator3-0.1 power-profiles-daemon upower libnotify-bin
```

---

## 3. Tray Icon Missing on Login

Ensure the user service is enabled:
```bash
systemctl --user status power-tray.service
systemctl --user enable --now power-tray.service
```

To view live logs:
```bash
journalctl --user -u power-tray.service -f
```

---

## 4. Customizing Display Refresh Rate (Non-ThinkPad L15)

On other laptops, `output.1.mode.2` might point to a resolution other than 48Hz.
Check your available modes using `kscreen-doctor`:
```bash
kscreen-doctor -o
```
Find the index of your lowest acceptable refresh rate mode (e.g. `output.eDP-1.mode.2` or `output.1.mode.2`), and adjust the call in `src/power-tray.cpp`.

---

## 5. Checking Hardware Registers Directly

To inspect current hardware states:
```bash
power-profile-manager status
```

Or read raw sysfs nodes:
```bash
cat /sys/devices/system/cpu/cpufreq/boost
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
cat /sys/devices/system/cpu/smt/control
cat /sys/class/drm/card1-eDP-1/amdgpu/panel_power_savings
cat /sys/module/pcie_aspm/parameters/policy
cat /sys/class/power_supply/BAT0/power_now
cat /sys/class/power_supply/BAT0/charge_control_end_threshold
```
