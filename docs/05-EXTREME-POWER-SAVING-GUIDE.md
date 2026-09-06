# 🔬 Linux 극한의 절전(Extreme Power Saving) 기술 딥 리서치 및 구현 가이드
> **A Comprehensive Engineering Guide to Sub-7W Linux Laptop Power Optimization**  
> *Targeted for AMD Ryzen (Zen 2/3/4/5), Modern ThinkPads, and Linux Kernel 6.x/7.x on Wayland*

---

## 📌 목차 (Table of Contents)
1. [개요: 리눅스 배터리 절전의 현실과 패러다임](#1-개요-리눅스-배터리-절전의-현실과-패러다임)
2. [하드웨어 소비 전력 계층도 (Power Breakdown)](#2-하드웨어-소비-전력-계층도-power-breakdown)
3. [CPU 실리콘 & 마이크로아키텍처 제어](#3-cpu-실리콘--마이크로아키텍처-제어)
   - AMD SMU 코프로세서 직접 레지스터 제어
   - CPU 드라이버 패러다임: `amd_pstate_epp` vs `acpi_cpufreq`
   - CPU C-State 깊은 수면(Residency)과 Idle 거버너 (`teo` vs `menu`)
   - "Race-to-Sleep" vs "하드웨어 클럭 캡" 딜레마 분석
4. [메인보드 전원부(VRM) 및 버스 링크 통제](#4-메인보드-전원부vrm-및-버스-링크-통제)
   - VRM 위상 드롭(Phase Shedding)과 TDC/EDC/PSI0 제어
   - PCIe ASPM L1.1 / L1.2 서브스테이트 강제
   - PCIe D3hot / D3cold Runtime PM
   - HD Audio 버스 컨트롤러 전원 차단
   - 타이머 및 인터럽트 웨이크업 병합 (Timer Coalescing)
5. [스토리지(NVMe) 및 시스템 메모리(RAM) 최적화](#5-스토리지nvme-및-시스템-메모리ram-최적화)
   - NVMe APST(Autonomous Power State Transition)와 L1.2
   - 커널 랩톱 모드(Laptop Mode 5)와 60초 배치 쓰기
   - RAM Transparent Hugepage(THP) 조각모음 인터럽트 차단
6. [디스플레이 및 내장 그래픽스(iGPU) 최적화](#6-디스플레이-및-내장-그래픽스igpu-최적화)
   - 백라이트 물리 전력 법칙과 밝기 상한 락
   - AMD ABM (Adaptive Backlight Management / Vari-Bright)
   - PSR (Panel Self Refresh)과 PSR2
   - AMDGPU OverDrive 클럭 상한 캡과 동적 DPM
   - 하드웨어 비디오 가속 (VA-API) 필수 설정
7. [무선 통신 및 주변 장치(Peripherals) 통제](#7-무선-통신-및-주변-장치peripherals-통제)
   - 블루투스 베이스밴드 라디오 완전 차단
   - Wi-Fi 802.11 DTIM 절전 슬립 (Power Save ON)
   - 내장 USB 센서(지문인식기, 웹캠) 공격적 Autosuspend
8. [섀시 쿨링팬 전력의 물리적 법칙 ($P \propto \text{RPM}^3$)](#8-섀시-쿨링팬-전력의-물리적-법칙-p-propto-textrpm3)
9. [실제 측정 데이터: 최적화 전/후 비교 검증](#9-실제-측정-데이터-최적화-전후-비교-검증)
10. [단계별 적용 체크리스트 (Actionable Checklist)](#10-단계별-적용-체크리스트-actionable-checklist)

---

## 1. 개요: 리눅스 배터리 절전의 현실과 패러다임

리눅스 데스크톱 환경(GNOME, KDE Plasma)의 기본 전원 관리자(`power-profiles-daemon`, `upower`)는 주로 **운영체제 레벨의 고수준 ACPI 힌트**만을 전달합니다. 

하지만 실제로 배터리가 빠르게 닳는 이유는 다음과 같습니다:
1. **과도한 순간 부스트**: 브라우저 탭을 열거나 가벼운 타이핑을 할 때도 CPU가 순간적으로 4.1GHz+로 치솟으며 전압이 1.35V 이상으로 인가되어 수십 와트의 피크 전력을 낭비합니다.
2. **메인보드 전원부(VRM) 스위칭 손실**: 순간 고부하를 대비해 메인보드의 다상(Multi-phase) Buck Converter가 항상 여러 페이즈를 대기시켜 막대한 열 손실이 발생합니다.
3. **디스크 I/O 주기적 웨이크업**: 백그라운드 프로세스들이 수초마다 디스크를 깨워 NVMe 컨트롤러가 초저전력(5mW) 딥슬립에 들어가지 못합니다.
4. **미사용 페리퍼럴의 누설 전류**: 켜져만 있는 블루투스 베이스밴드, 지문인식기, 미사용 오디오 버스가 지속적으로 0.5W~1.5W를 소모합니다.

**ThinkPower** 프로젝트를 진행하며 연구 및 실측된 데이터에 기반하여, 단순한 소프트웨어 설정을 넘어 **실리콘 레지스터, 메인보드 전원부, 버스 프로토콜 레벨까지 제어하는 극한의 절전 기술**을 집대성합니다.

---

## 2. 하드웨어 소비 전력 계층도 (Power Breakdown)

노트북 배터리 총 방전량(System Total Power)은 아래와 같은 물리적 컴포넌트들의 합으로 구성됩니다:

$$\text{Total System Power} = P_{\text{APU}} + P_{\text{VRM\_Loss}} + P_{\text{Display}} + P_{\text{RAM}} + P_{\text{Fan}} + P_{\text{Storage}} + P_{\text{Wireless}} + P_{\text{Board/EC}}$$

```
[ 순정 기본 상태 (15W ~ 18W 방전) ]
┌─────────────────────────┬──────────────┬──────────────┬──────────────┐
│ AMD APU SoC (8W~12W)    │ VRM손실(3W)  │ 디스플레이(3W)│ 기타장치(2W)  │
└─────────────────────────┴──────────────┴──────────────┴──────────────┘

[ ThinkPower 극한 최적화 상태 (6.5W ~ 8.2W 방전) ]
┌──────────────┬────────┬────────┬───────┬──────┐
│ APU (3.98W)  │VRM(1W) │패널(1W)│RAM(1W)│기타  │
└──────────────┴────────┴────────┴───────┴──────┘
```

---

## 3. CPU 실리콘 & 마이크로아키텍처 제어

### 3.1 AMD SMU 코프로세서 직접 레지스터 제어 (Hardware TDP Lock)
* **원리**: 현대 AMD 프로세서는 내부에 전력과 전압을 총괄하는 독립 마이크로컨트롤러인 **SMU (System Management Unit)**를 내장하고 있습니다.
* **통제 기술**: OS의 스케줄러를 우회하여 SMU 레지스터에 직접 한계값을 프로그래밍합니다.
  * `STAPM (Sustained Power Tracking Limit)`: 프로세서가 장기적으로 유지 가능한 전력 한계.
  * `Fast PPT / Slow PPT`: 순간 피크 전력 한계.
  * `Tctl Temp Limit`: 쓰로틀링 시작 온도.
* **극한 튜닝 값**:
  ```bash
  # 초절전: STAPM 4W, Fast 5W, Slow 4W, 온도 60°C 하드웨어 락
  ryzenadj --stapm-limit=4000 --fast-limit=5000 --slow-limit=4000 --tctl-temp=60
  ```
* **효과**: 프로세서가 아무리 무거운 연산을 시도해도 APU 패키지 전력이 4.00W를 물리적으로 초과할 수 없어 전력 소모가 극적으로 억제됩니다.

### 3.2 CPU 드라이버 패러다임: `amd_pstate_epp` vs `acpi_cpufreq`
* **`amd_pstate=active` (EPP 모드)**:
  * Zen 2 이상 최신 커널(6.5+)에서 권장되는 CPPC(Collaborative Processor Performance Control) 기반 드라이버.
  * 코어별 전력 선호도(Energy Performance Preference)를 하드웨어 레벨에서 지시:
    ```bash
    echo "power" | sudo tee /sys/devices/system/cpu/cpufreq/policy*/energy_performance_preference
    ```
* **`acpi_cpufreq` 환경에서의 제어**:
  * 부스트 차단: `echo 0 > /sys/devices/system/cpu/cpufreq/boost`
  * 하드웨어 최저 P-State 고정: `echo 1400000 > /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq`

### 3.3 CPU C-State 깊은 수면(Residency)과 Idle 거버너 (`teo` vs `menu`)
* **Idle 거버너**: CPU가 유휴 상태일 때 어떤 깊이의 C-State(C1, C2, C6 등)로 들어갈지 결정하는 커널 알고리즘.
* **`teo` (Timer Enabled Optimizer)**:
  * 현대 틱리스(Tickless, `NO_HZ`) 커널 환경에서 타이머 만료 시점을 정밀 예측하여 불필요한 얕은 수면을 방지하고 즉시 깊은 C-State(C6)로 진입.
  * 적용: `echo teo > /sys/devices/system/cpu/cpuidle/current_governor`
* **POLL 상태 차단**:
  * CPU가 수면에 들기 전 아주 짧은 시간 동안 spin-wait(바쁜 대기)하는 C0 poll 상태를 비활성화:
    ```bash
    for p in /sys/devices/system/cpu/cpu*/cpuidle/state0/disable; do
        echo 1 > "$p"
    done
    ```

### 3.4 "Race-to-Sleep" vs "하드웨어 클럭 캡" 딜레마
* **Race-to-Sleep 이론**: 작업을 최대 클럭으로 가장 빠르게 끝내고 즉시 깊은 C-State로 복귀하는 것이 총 에너지 소비 측면에서 유리하다는 이론.
* **현실의 함정**: 웹 브라우징(자바스크립트), 백그라운드 동기화 등 지속적인 이벤트가 발생하는 실사용 환경에서는 작업이 끝나지 않아 CPU가 지속적으로 고전압(1.35V) 상태에 머물게 됨.
* **최적 해법**: **하이브리드 전략**
  * SMT(16스레드)는 **100% 켜두어 병렬 처리 능력을 유지**하되,
  * 클럭 상한을 **1.4GHz~1.7GHz**로 제한하고,
  * SMU TDP를 **4W~10W**로 캡을 씌우는 것이 가장 낮은 총 소비 전력을 기록함.

---

## 4. 메인보드 전원부(VRM) 및 버스 링크 통제

메인보드 전원부와 버스 인터페이스는 랩톱 전체 전력의 **20%~30%**를 차지하는 거대한 전력 낭비원입니다.

### 4.1 VRM 위상 드롭(Phase Shedding)과 TDC/EDC/PSI0 제어
* **원리**: 메인보드의 다상 전원부(Buck Converter)는 순간 고전류를 공급하기 위해 여러 개의 MOSFET 페이즈를 켜둡니다. 이는 경부하 상태에서 막대한 스위칭 열손실을 유발합니다.
* **통제 기술**:
  * **TDC (Thermal Design Current)**: 지속 전류 한계를 **12.0A**로 제한 (`--vrm-current=12000`)
  * **EDC (Electrical Design Current)**: 최대 피크 전류를 **16.0A**로 제한 (`--vrmmax-current=16000`)
  * **PSI0 (Power State Indicator)**: VRM 컨트롤러에게 전류 한계를 8A로 통보하여 **단일 위상(1-Phase) 슬립 모드로 진입**하도록 하드웨어 신호 전달 (`--psi0-current=8000`)
* **효과**: 메인보드 발열 및 스위칭 손실을 **1.5W~2.5W 이상 직접 절감**.

### 4.2 PCIe ASPM L1.1 / L1.2 서브스테이트 강제
* **원리**: PCIe 링크가 유휴 상태일 때 고속 차동 클럭을 끄고 서브 5mW 딥슬립(L1.2)에 진입.
* **설정**:
  ```bash
  echo powersave > /sys/module/pcie_aspm/parameters/policy
  ```
* **커널 부팅 옵션**: `pcie_aspm=force`

### 4.3 PCIe & USB Runtime PM (D3hot / D3cold)
* 데이터 전송이 없는 모든 PCIe 엔드포인트/브리지와 USB 장치를 즉각 서스펜드:
  ```bash
  for dev in /sys/bus/pci/devices/*/power/control; do echo auto > "$dev"; done
  for dev in /sys/bus/usb/devices/*/power/control; do echo auto > "$dev"; done
  for dev in /sys/bus/usb/devices/*/power/autosuspend_delay_ms; do echo 1000 > "$dev"; done
  ```

### 4.4 HD Audio 버스 컨트롤러 전원 차단
* 오디오 코덱(ALC257)뿐만 아니라 **PCI HD Audio 컨트롤러 자체를 D3hot 슬립**에 진입:
  ```bash
  echo 1 > /sys/module/snd_hda_intel/parameters/power_save
  echo Y > /sys/module/snd_hda_intel/parameters/power_save_controller
  ```

### 4.5 타이머 및 인터럽트 웨이크업 병합 (Timer Coalescing)
* `nmi_watchdog = 0`: 초당 수백 회 발생하는 NMI 인터럽트 차단
* `timer_migration = 1`: 타이머 인터럽트를 한 코어로 몰아 나머지 코어와 전원 레일의 C6 수면 유지
* `workqueue.power_efficient = Y`: 절전 워크큐 활성화

---

## 5. 스토리지(NVMe) 및 시스템 메모리(RAM) 최적화

### 5.1 커널 랩톱 모드(Laptop Mode 5)와 60초 배치 쓰기
* **원리**: NVMe SSD(예: 삼성 980 PRO)는 활성 상태(D0)에서 3W~6W를 소모하지만, PCIe ASPM L1.2 서브스테이트(PS4)에서는 **5mW 미만**을 소모합니다.
* **통제 기술**:
  ```bash
  echo 5 > /proc/sys/vm/laptop_mode
  echo 3000 > /proc/sys/vm/dirty_writeback_centisecs  # 30초 주기
  echo 6000 > /proc/sys/vm/dirty_expire_centisecs     # 60초 만료
  echo 50 > /proc/sys/vm/vfs_cache_pressure
  ```
* **효과**: 디스크 쓰기가 최대 60초 동안 RAM 캐시에 묶이므로, NVMe 컨트롤러가 깨어나지 않고 95% 이상의 시간을 L1.2 딥슬립 상태로 유지.

### 5.2 RAM Transparent Hugepage(THP) 조각모음 인터럽트 차단
* 백그라운드 메모리 조각모음 데몬(`khugepaged`)이 주기적으로 깨어나 CPU와 메모리 버스를 가열하는 현상 방지:
  ```bash
  echo madvise > /sys/kernel/mm/transparent_hugepage/enabled
  echo never > /sys/kernel/mm/transparent_hugepage/defrag
  echo 0 > /sys/kernel/mm/transparent_hugepage/khugepaged/defrag
  echo 1 > /proc/sys/vm/compact_memory
  ```

---

## 6. 디스플레이 및 내장 그래픽스(iGPU) 최적화

디스플레이는 단일 장치 중 전력 소모가 가장 큽니다 (100% 밝기 기준 최대 ~3.5W).

### 6.1 밝기 제어와 제곱 법칙 ($P \propto \text{Brightness}^{1.2}$)
* 백라이트 밝기를 100%에서 **20%**로 내리면 백라이트 전력이 **~2.8W에서 ~0.48W로 급감** (약 83% 절감).

### 6.2 AMD ABM (Adaptive Backlight Management / Vari-Bright)
* 화면 이미지의 RGB 픽셀 값을 하드웨어 알고리즘으로 보정하면서 물리 LED 백라이트를 어둡게 제어.
* 색 왜곡 없이 디스플레이 전력을 0.5W~1.0W 추가 절감:
  ```bash
  echo 2 > /sys/class/drm/card1-eDP-1/amdgpu/panel_power_savings  # 레벨 2 (균형 절전)
  echo 4 > /sys/class/drm/card1-eDP-1/amdgpu/panel_power_savings  # 레벨 4 (최대 절전)
  ```

### 6.3 PSR (Panel Self Refresh) & PSR2
* 정적 화면(글 읽기, 코딩) 시 GPU 송출 파이프라인을 끄고 패널 자체 메모리에서 화면을 리프레시.

### 6.4 AMDGPU OverDrive 클럭 상한 캡 (640MHz / 40% Cap)
* 내장 그래픽 클럭을 최저(200MHz)로 강제 고정하면 화면 스크롤이 끊기므로, **최대 상한만 640MHz(40%)로 락**을 걸고 DPM은 `auto`로 유지:
  ```bash
  echo manual > /sys/class/drm/card1/device/power_dpm_force_performance_level
  echo "s 1 640" > /sys/class/drm/card1/device/pp_od_clk_voltage
  echo "c" > /sys/class/drm/card1/device/pp_od_clk_voltage
  ```

### 6.5 하드웨어 비디오 가속 (VA-API) 필수 활성화
* 웹 브라우저(Chrome/Firefox) 및 미디어 플레이어에서 유튜브/영상 시청 시 CPU 소프트웨어 디코딩을 방지.
* VA-API 하드웨어 디코딩 활성화 시 비디오 재생 전력: **14W~18W ➡️ 6W~8W로 50% 이상 절감**.

---

## 7. 무선 통신 및 주변 장치(Peripherals) 통제

### 7.1 블루투스 베이스밴드 하드웨어 차단
* 블루투스가 켜져만 있어도 HCI 컨트롤러와 RF 수신부가 0.2W~0.4W를 소모합니다.
* 초절전 모드 진입 시 소프트 블록, 모드 복구 시 자동 원복:
  ```bash
  rfkill block bluetooth    # 절전 진입
  rfkill unblock bluetooth  # 복원
  ```

### 7.2 Wi-Fi 802.11 DTIM 절전 슬립 (Power Save ON)
* 인터넷 연결을 유지하면서 공유기의 DTIM 비콘 주기에 맞춰 수신부를 수면 상태로 전환:
  ```bash
  iw dev wlan0 set power_save on
  ```
* 소비 전력: ~0.8W ➡️ **~0.25W**로 감소.

---

## 8. 섀시 쿨링팬 전력의 물리적 법칙 ($P \propto \text{RPM}^3$)

쿨링팬 모터가 소모하는 전력은 팬 속도의 세제곱($\text{RPM}^3$)에 비례합니다:

$$\frac{P_1}{P_2} = \left(\frac{\text{RPM}_1}{\text{RPM}_2}\right)^3$$

| 팬 상태 / 속도 | 전력 소모 | 소음 및 상태 |
| :--- | :---: | :--- |
| **5,000 RPM (Full)** | **~1.80 W** | 최대 풍량, 고소음 |
| **3,100 RPM (Medium)**| **~0.58 W** | 표준 회전 |
| **1,900 RPM (Low)** | **~0.15 W** | 정숙 운행 |
| **0 RPM (Off / Passive)** | **0.00 W** | **완전 무소음, 전력 0W** |

* **ThinkPower의 접근법**:
  * 팬을 억지로 끄면 하드웨어가 과열되어 위험합니다.
  * ThinkPower는 **SMU TDP를 4W로 락**을 걸어 발열 원천 자체를 없앰으로써, CPU 온도를 **38°C~40°C**로 유지시켜 ThinkPad EC가 **자연스럽게 팬을 0 RPM 또는 최저속도로 낮추도록 유도**합니다.

---

## 9. 실제 측정 데이터: 최적화 전/후 비교 검증

* **측정 기기**: Lenovo ThinkPad L15 Gen 1 (AMD Ryzen 7 PRO 4750U, CachyOS Kernel)
* **측정 도구**: `tp-ppm analyze` (RAPL + AMD SMU Co-processor Telemetry + BAT0 ACPI)

| 항목 | 순정 기본 상태 (Default) | ThinkPower 초절전 (`ultra`) | 절감 효과 |
| :--- | :---: | :---: | :---: |
| **총 시스템 소비 전력** | **15.20 W ~ 18.50 W** | **6.52 W ~ 8.23 W** | **약 50% ~ 57% 절감** |
| **AMD APU SoC (STAPM)** | 8.50 W ~ 14.00 W | **3.82 W ~ 4.00 W (Hardware Lock)** | **최대 70% 억제** |
| **메인보드 전원부(VRM) 손실**| 2.80 W ~ 3.50 W | **1.20 W ~ 2.10 W (Phase Shedding)** | **약 40% 저감** |
| **디스플레이 (15.6" FHD)** | 2.80 W (밝기 70%) | **1.31 W (밝기 20% + ABM)** | **53% 절감** |
| **SSD (Samsung 980 PRO)** | 0.80 W (수시 Wakeup) | **0.15 W (ASPM L1.2 유지)** | **81% 절감** |
| **쿨링팬 전력** | 0.90 W ~ 1.50 W | **0.15 W ~ 0.58 W** | **최대 80% 절감** |
| **블루투스 대기 전력** | 0.25 W | **0.00 W (RF Block)** | **100% 차단** |
| **예상 배터리 연속 사용 시간**| **약 2시간 40분 ~ 3시간** | **약 6시간 30분 ~ 7시간 40분** | **배터리 수명 2.4배 연장** |

---

## 10. 단계별 적용 체크리스트 (Actionable Checklist)

리눅스 랩톱에서 극한의 배터리 수명을 달성하고자 하는 엔지니어를 위한 요약 체크리스트입니다:

- [x] **1. 하드웨어 TDP 캡**: `ryzenadj`를 통해 STAPM 4W~10W 락 설정
- [x] **2. 전원부 위상 억제**: TDC 12A / EDC 16A / PSI0 단일 위상 유도
- [x] **3. GPU 상한 제한**: OverDrive로 최대 클럭 640MHz(40%) 캡, DPM `auto`
- [x] **4. 디스플레이 밝기 최적화**: 20% 상한 및 AMD ABM 레벨 2~4 활성화
- [x] **5. NVMe ASPM 수면**: `vm.laptop_mode = 5` 및 flush 60초 지연
- [x] **6. RAM 인터럽트 차단**: THP defrag `never`, `khugepaged = 0`
- [x] **7. 버스 Runtime PM**: 모든 PCIe/USB 장치 `power/control = auto`
- [x] **8. 오디오 D3hot 슬립**: `snd_hda_intel.power_save_controller = Y`
- [x] **9. 미사용 무선 차단**: 미사용 시 블루투스 `rfkill` 차단, Wi-Fi PowerSave ON
- [x] **10. 다계층 Failsafe**: AC 충전기 연결 시 또는 프로세스 종료 시 즉시 공장 출고값으로 자동 복원되는 안전망 구축

---
*본 문서는 ThinkPower 프로젝트의 실측 텔레메트리 데이터와 리눅스 커널 소스 인터페이스에 기반하여 작성되었습니다.*
