# 📖 Features & User Guide

ThinkPower provides an all-in-one replacement for the default KDE Plasma battery widget with advanced hardware tuning, peripheral monitoring, and battery conservation features.

---

## 1. 4-Stage Power Profiles

| Profile | Target Frequency | Thread Count | Refresh Rate | ABM Level | Fan / Heat | Primary Use Case |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **⚡ Performance** | Up to 4.1GHz (Boost) | 16 Threads | 60Hz | 0 (Off) | High / Active | Compilation, heavy computing |
| **⚖️ Balanced** | Up to 4.1GHz (Dynamic) | 16 Threads | 60Hz | 1 (Subtle) | Moderate | Normal desktop usage / AC power |
| **🍃 Smart Save** | **1.7GHz Ceiling** | **16 Threads** | **60Hz** | **2 (Balanced)** | **Silent / Low** | **Daily battery work (100% smooth)** |
| **🛡️ Ultra Save** | **1.4GHz Locked** | **8 Cores (SMT Off)**| **48Hz** | **4 (Max)** | **0 RPM (Off)** | **Emergency battery extension** |

> **⭐ Connectivity Guarantee**:
> In both **Smart Save** and **Ultra Save**, Wi-Fi and Bluetooth connections are **never** disabled. Peripherals, headphones, and internet access remain 100% functional.

---

## 2. Real-Time Wattage & Flow Direction (`+` / `-`)

ThinkPower uses electrical engineering sign conventions:
- **`+` (Power Inflow / Charging)**: When plugged into AC, shows the rate entering the battery cell (e.g. `⚡ 45% (+31.2W)`).
- **`-` (Power Outflow / Discharging)**: When running on battery, shows the real system discharge rate (e.g. ` 45% (-12.8W)`).
- **`0.0W` (Full / Standby)**: When the battery reaches threshold or 100%, indicates AC pass-through.

---

## 3. ThinkPad Battery Conservation Mode (Charge Threshold)

ThinkPad laptops feature hardware battery protection thresholds (typically 80%) to prevent lithium-ion cell degradation from prolonged 100% trickle-charging.

- **Threshold Detection**: Reads `/sys/class/power_supply/BAT0/charge_control_end_threshold`.
- **Accurate Time Estimation**: Instead of calculating time to 100%, ThinkPower calculates the remaining time to reach the **protection threshold** (e.g. `80%까지 약 25분 남음`).
- **Status Indicator**: Displays `🛡️ 보호한도 : 80% (수명 보호)` in the tooltip and menu.

---

## 4. Bluetooth & Wireless Peripheral Battery Monitoring

Integrated with UPower and BlueZ to monitor all connected wireless peripherals:
- 🎧 **Earphones / Headsets**: AirPods, Galaxy Buds, QCY, Sony, etc.
- 🖱️ **Wireless Mice**: Logitech MX Master, Bluetooth mice.
- ⌨️ **Wireless Keyboards**: Keychron, Apple Magic Keyboard, etc.
- 🎮 **Game Controllers**: Xbox Wireless Controller, DualSense.

Displays in both the hover tooltip and dropdown menu with real-time percentage updates.

---

## 5. Non-Wrapping (Clean HUD) Tooltip

Designed specifically for KDE Plasma's StatusNotifierItem popup width:
- All metric lines are constrained under 38 characters.
- Colons and symbols align vertically.
- Never wraps onto awkward secondary lines.

```text
┌──────────────────────────────────────────────────┐
│  🔋 배터리 76% (충전 중)                         │
│                                                  │
│  ⚡ 충전량 : +21.5W                               │
│  ⏳ 충전예상 : 80%까지 약 5분                     │
│  ⚙️ 전원모드 : 🍃 스마트 절전 (1.7GHz)             │
│  🛡️ 보호한도 : 80% (수명 보호)                   │
│  🩺 배터리건강 : 94.2% (88회)                     │
│  🎧 Bluetooth Earphones : 80%                    │
│  📡 무선상태 : Wi-Fi · BT On                     │
└──────────────────────────────────────────────────┘
```

---

## 6. Automatic Keyboard Backlight State Restoration

- When entering **Ultra Save**, the current keyboard backlight brightness level (`1` or `2`) is cached in `~/.cache/prev_kbd_backlight` before turning off the LED.
- When switching back to **Smart Save**, **Balanced**, or **Performance**, the cached brightness is automatically restored.
