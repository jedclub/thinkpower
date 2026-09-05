#!/usr/bin/env python3
import gi
gi.require_version('Gtk', '3.0')
gi.require_version('AyatanaAppIndicator3', '0.1')
from gi.repository import Gtk, AyatanaAppIndicator3, GLib
import subprocess
import os
import shutil
import dbus
from dbus.mainloop.glib import DBusGMainLoop

APPINDICATOR_ID = 'power_profile_indicator'
CACHE_DIR = os.environ.get('XDG_CACHE_HOME', os.path.expanduser('~/.cache'))
STATE_FILE = os.path.join(CACHE_DIR, 'power_profile_mode')

def get_manager_bin():
    path = shutil.which('power-profile-manager')
    if path:
        return path
    for candidate in [
        '/usr/local/bin/power-profile-manager',
        os.path.expanduser('~/.local/bin/power-profile-manager')
    ]:
        if os.path.exists(candidate) and os.access(candidate, os.X_OK):
            return candidate
    return 'power-profile-manager'

class PowerTrayApp:
    def __init__(self):
        self.updating_ui = False
        self.indicator = AyatanaAppIndicator3.Indicator.new(
            APPINDICATOR_ID,
            'battery-profile-balanced-symbolic',
            AyatanaAppIndicator3.IndicatorCategory.HARDWARE
        )
        self.indicator.set_status(AyatanaAppIndicator3.IndicatorStatus.ACTIVE)
        self.menu = Gtk.Menu()
        self.build_menu()
        self.indicator.set_menu(self.menu)

        # 1. 초기 UI 및 상태 복원
        self.update_state_ui()

        # 2. 시스템 D-Bus (power-profiles-daemon) 이벤트 리스너 등록
        self.init_dbus_listener()

        # 3. 2초마다 배터리 잔량, 충방전 전력(W), 시간 주기적 갱신
        GLib.timeout_add_seconds(2, self.check_state_sync)

    def init_dbus_listener(self):
        try:
            DBusGMainLoop(set_as_default=True)
            bus = dbus.SystemBus()
            bus.add_signal_receiver(
                self.on_system_profile_changed,
                signal_name="PropertiesChanged",
                dbus_interface="org.freedesktop.DBus.Properties",
                path="/net/hadess/PowerProfiles"
            )
        except Exception as e:
            print(f"DBus listener init error: {e}")

    def on_system_profile_changed(self, interface, changed_props, invalidated_props):
        """KDE 기본 위젯이나 시스템 이벤트로 인해 OS 프로파일이 바뀌었을 때 자동 연동"""
        if "ActiveProfile" in changed_props:
            sys_profile = str(changed_props["ActiveProfile"])
            current = self.get_current_mode()

            if sys_profile == "performance" and current != "performance":
                self.switch_mode("performance", trigger_source="system")
            elif sys_profile == "balanced" and current != "balanced":
                self.switch_mode("balanced", trigger_source="system")
            elif sys_profile == "power-saver":
                if current not in ["save", "ultra"]:
                    self.switch_mode("save", trigger_source="system")

    def get_current_mode(self):
        if os.path.exists(STATE_FILE):
            try:
                with open(STATE_FILE, 'r') as f:
                    return f.read().strip()
            except Exception:
                pass
        return 'balanced'

    def set_current_mode(self, mode):
        try:
            with open(STATE_FILE, 'w') as f:
                f.write(mode + '\n')
        except Exception:
            pass

    def get_peripheral_batteries(self):
        """연결된 블루투스 기기 잔량 조회 (UPower 연동)"""
        peripherals = []
        try:
            bus = dbus.SystemBus()
            upower_obj = bus.get_object('org.freedesktop.UPower', '/org/freedesktop/UPower')
            upower = dbus.Interface(upower_obj, 'org.freedesktop.UPower')
            devices = upower.EnumerateDevices()

            for d in devices:
                dev_obj = bus.get_object('org.freedesktop.UPower', d)
                props = dbus.Interface(dev_obj, 'org.freedesktop.DBus.Properties').GetAll('org.freedesktop.UPower.Device')
                dev_type = int(props.get('Type', 0))
                is_present = bool(props.get('IsPresent', True))

                if dev_type not in [1, 2, 3] and is_present:
                    model = str(props.get('Model', '')).strip()
                    if not model:
                        model = str(props.get('NativePath', '')).split('/')[-1]
                    pct = float(props.get('Percentage', 0.0))

                    icon = '🔋'
                    if dev_type in [17, 18, 19]:
                        icon = '🎧'
                    elif dev_type == 5:
                        icon = '🖱️'
                    elif dev_type == 6:
                        icon = '⌨️'
                    elif dev_type == 12:
                        icon = '🎮'

                    peripherals.append({
                        'name': model,
                        'pct': int(round(pct)),
                        'icon': icon
                    })
        except Exception:
            pass
        return peripherals

    def get_battery_info(self):
        bat_dir = '/sys/class/power_supply/BAT0'
        capacity = 0
        status = 'Unknown'
        power_w = 0.0
        time_str = ''
        cycle_count = 0
        health_pct = 100.0
        charge_limit = 100
        start_threshold = 0

        if os.path.exists(bat_dir):
            try:
                with open(os.path.join(bat_dir, 'capacity'), 'r') as f:
                    capacity = int(f.read().strip())
                with open(os.path.join(bat_dir, 'status'), 'r') as f:
                    status = f.read().strip()

                energy_now = 0
                power_now = 0
                energy_full = 0
                energy_full_design = 0

                if os.path.exists(os.path.join(bat_dir, 'energy_now')):
                    with open(os.path.join(bat_dir, 'energy_now'), 'r') as f:
                        energy_now = float(f.read().strip())
                if os.path.exists(os.path.join(bat_dir, 'power_now')):
                    with open(os.path.join(bat_dir, 'power_now'), 'r') as f:
                        power_now = float(f.read().strip())
                if os.path.exists(os.path.join(bat_dir, 'energy_full')):
                    with open(os.path.join(bat_dir, 'energy_full'), 'r') as f:
                        energy_full = float(f.read().strip())
                if os.path.exists(os.path.join(bat_dir, 'energy_full_design')):
                    with open(os.path.join(bat_dir, 'energy_full_design'), 'r') as f:
                        energy_full_design = float(f.read().strip())
                if os.path.exists(os.path.join(bat_dir, 'cycle_count')):
                    with open(os.path.join(bat_dir, 'cycle_count'), 'r') as f:
                        cycle_count = int(f.read().strip())

                if os.path.exists(os.path.join(bat_dir, 'charge_control_end_threshold')):
                    with open(os.path.join(bat_dir, 'charge_control_end_threshold'), 'r') as f:
                        charge_limit = int(f.read().strip())
                elif os.path.exists(os.path.join(bat_dir, 'charge_stop_threshold')):
                    with open(os.path.join(bat_dir, 'charge_stop_threshold'), 'r') as f:
                        charge_limit = int(f.read().strip())

                if os.path.exists(os.path.join(bat_dir, 'charge_control_start_threshold')):
                    with open(os.path.join(bat_dir, 'charge_control_start_threshold'), 'r') as f:
                        start_threshold = int(f.read().strip())

                if energy_full_design > 0 and energy_full > 0:
                    health_pct = (energy_full / energy_full_design) * 100.0

                if power_now > 0:
                    power_w = power_now / 1000000.0

                    def fmt_dur(hrs):
                        h = int(hrs)
                        m = int(round((hrs - h) * 60))
                        if m >= 60:
                            h += 1
                            m = 0
                        if h > 0 and m > 0:
                            return f"{h}시간 {m}분"
                        elif h > 0:
                            return f"{h}시간"
                        elif m > 0:
                            return f"{m}분"
                        else:
                            return "1분 미만"

                    if status == 'Discharging' and energy_now > 0:
                        hours = energy_now / power_now
                        time_str = f"약 {fmt_dur(hours)}"
                    elif status == 'Charging':
                        target_energy = energy_full * (charge_limit / 100.0)
                        diff = max(0, target_energy - energy_now)
                        if diff > 0:
                            hours = diff / power_now
                            dur = fmt_dur(hours)
                            if charge_limit < 100:
                                time_str = f"{charge_limit}%까지 약 {dur}"
                            else:
                                time_str = f"완충까지 약 {dur}"
                        else:
                            time_str = f"{charge_limit}% 도달 직전"
            except Exception:
                pass

        if charge_limit < 100:
            protect_str = f"{charge_limit}% (수명 보호)"
        else:
            protect_str = "100% (일반)"

        return {
            'capacity': capacity,
            'status': status,
            'power_w': power_w,
            'time_str': time_str,
            'cycle_count': cycle_count,
            'health_pct': health_pct,
            'charge_limit': charge_limit,
            'protect_str': protect_str,
            'peripherals': self.get_peripheral_batteries()
        }

    def build_menu(self):
        self.header_battery = Gtk.MenuItem(label="🔋 배터리 정보 로딩 중...")
        self.header_battery.set_sensitive(False)
        self.header_battery.show()
        self.menu.append(self.header_battery)

        self.header_power = Gtk.MenuItem(label="⚡ 전력 정보 로딩 중...")
        self.header_power.set_sensitive(False)
        self.header_power.show()
        self.menu.append(self.header_power)

        self.header_protect = Gtk.MenuItem(label="🛡️ 배터리 보호 로딩 중...")
        self.header_protect.set_sensitive(False)
        self.header_protect.show()
        self.menu.append(self.header_protect)

        self.header_bt = Gtk.MenuItem(label="🎧 블루투스 기기 조회 중...")
        self.header_bt.set_sensitive(False)
        self.header_bt.show()
        self.menu.append(self.header_bt)

        sep1 = Gtk.SeparatorMenuItem()
        sep1.show()
        self.menu.append(sep1)

        self.group = None
        self.items = {}

        modes = [
            ('performance', '⚡ 성능 (4.1GHz 부스트)'),
            ('balanced',    '⚖️ 균형 (표준 동적 클럭)'),
            ('save',        '🍃 스마트 절전 (1.7GHz 상한, 16T 유지)'),
            ('ultra',       '🛡️ 극한 초절전 (1.4GHz, 48Hz, 백라이트MAX)')
        ]

        for key, label in modes:
            item = Gtk.RadioMenuItem.new_with_label_from_widget(self.group, label)
            if self.group is None:
                self.group = item
            item.connect('toggled', self.on_radio_toggled, key)
            self.menu.append(item)
            self.items[key] = item
            item.show()

        sep2 = Gtk.SeparatorMenuItem()
        sep2.show()
        self.menu.append(sep2)

        detail_item = Gtk.MenuItem(label="📊 하드웨어 상세 정보 보기 (알림창)")
        detail_item.connect('activate', self.on_show_detailed_status)
        detail_item.show()
        self.menu.append(detail_item)

    def notify_user(self, title, msg, icon="battery-profile-powersave-symbolic"):
        try:
            subprocess.Popen(['notify-send', '-t', '4000', '-i', icon, title, msg])
        except Exception:
            pass

    def on_radio_toggled(self, widget, mode_key):
        if self.updating_ui:
            return
        if widget.get_active():
            self.switch_mode(mode_key, trigger_source="user")

    def switch_mode(self, mode_key, trigger_source="user"):
        self.set_current_mode(mode_key)
        self.update_state_ui()

        if mode_key == 'ultra':
            subprocess.Popen(['kscreen-doctor', 'output.1.mode.2'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.Popen(['balooctl6', 'suspend'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if trigger_source == "user":
                self.notify_user(
                    "전원 프로파일: [⚡ 극한 초절전 ON]",
                    "시스템: 절전 모드 | 1.4GHz 고정 | 48Hz 다운클럭 | 백라이트MAX\n(※ Wi-Fi와 블루투스는 정상 유지됩니다)",
                    "battery-low"
                )
        else:
            subprocess.Popen(['kscreen-doctor', 'output.1.mode.1'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.Popen(['balooctl6', 'resume'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

            if trigger_source == "user":
                if mode_key == 'save':
                    self.notify_user("전원 프로파일: [스마트 절전]", "시스템: 절전 모드 | 1.7GHz 상한 및 백라이트 최적화\n(Wi-Fi, BT, 16스레드 100% 정상 작동)", "battery-profile-powersave-symbolic")
                elif mode_key == 'balanced':
                    self.notify_user("전원 프로파일: [균형]", "시스템: 균형 모드 | 표준 동적 클럭", "battery-profile-balanced-symbolic")
                elif mode_key == 'performance':
                    self.notify_user("전원 프로파일: [성능]", "시스템: 성능 모드 | 4.1GHz CPU 부스트 활성화", "battery-profile-performance-symbolic")

        os_target_profile = {
            'performance': 'performance',
            'balanced':    'balanced',
            'save':        'power-saver',
            'ultra':       'power-saver'
        }.get(mode_key, 'balanced')

        subprocess.Popen(['powerprofilesctl', 'set', os_target_profile], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        cmd = [get_manager_bin(), mode_key]
        subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def update_icon_label_and_tooltip(self, mode, bat_info):
        cap = bat_info['capacity']
        status = bat_info['status']
        power_w = bat_info['power_w']
        time_str = bat_info['time_str']
        health_pct = bat_info['health_pct']
        cycle_count = bat_info['cycle_count']
        protect_str = bat_info['protect_str']
        charge_limit = bat_info['charge_limit']
        peripherals = bat_info['peripherals']

        # 1. 작업표시줄 라벨 (충전 +, 방전 -)
        if status == 'Charging':
            label_text = f" ⚡ {cap}% (+{power_w:.1f}W)"
        elif status == 'Full' or (status == 'Not charging' and cap >= charge_limit):
            label_text = f" 🔌 {cap}% (대기)"
        else:
            label_text = f" {cap}% (-{power_w:.1f}W)"

        self.indicator.set_label(label_text, " ⚡ 100% (+00.0W)")

        # 2. 아이콘 선택
        pct_10 = min(100, max(0, int(round(cap / 10.0) * 10)))
        prof_map = {
            'performance': 'performance',
            'balanced':    'balanced',
            'save':        'powersave',
            'ultra':       'powersave'
        }
        p_name = prof_map.get(mode, 'balanced')

        if status == 'Charging':
            icon_name = f"battery-{pct_10:03d}-charging-profile-{p_name}"
        else:
            icon_name = f"battery-{pct_10:03d}-profile-{p_name}"

        self.indicator.set_icon_full(icon_name, f"{cap}% - {mode}")

        # 3. 마우스 호버(Hover) 시 나타나는 툴팁 (워드랩 완전 방지: 컴팩트 1줄 타이틀 및 본문 최적화)
        status_ko = "충전 중" if status == 'Charging' else ("배터리 사용" if status == 'Discharging' else "충전 완료")

        mode_titles = {
            'performance': '⚡ 성능 (4.1GHz)',
            'balanced':    '⚖️ 균형 (동적클럭)',
            'save':        '🍃 스마트 절전 (1.7GHz)',
            'ultra':       '🛡️ 극한 초절전 (1.4GHz, 48Hz)'
        }
        mode_title = mode_titles.get(mode, mode)

        time_line = ""
        if status == 'Charging':
            power_line = f"⚡ 충전량 : +{power_w:.1f}W"
            if time_str:
                time_line = f"⏳ 충전예상 : {time_str}"
        elif status == 'Full' or (status == 'Not charging' and cap >= charge_limit):
            power_line = f"🔌 전원 : 어댑터 직결 ({charge_limit}% 대기)"
        else:
            power_line = f"⚡ 사용량 : -{power_w:.1f}W"
            if time_str:
                time_line = f"⏳ 남은시간 : {time_str}"

        protect_line = f"🛡️ 보호한도 : {protect_str}"
        health_line = f"🩺 배터리건강 : {health_pct:.1f}% ({cycle_count}회)"

        periph_lines = []
        for p in peripherals:
            periph_lines.append(f"{p['icon']} {p['name']} : {p['pct']}%")

        # 툴팁 제목은 시스템 대형 폰트에서도 절대 2줄로 래핑되지 않도록 1줄로 컴팩트하게 제한
        tooltip_title = f"배터리 {cap}% ({status_ko})"

        body_elements = [power_line]
        if time_line:
            body_elements.append(time_line)
        body_elements.extend([
            f"⚙️ 전원모드 : {mode_title}",
            protect_line,
            health_line
        ])
        if periph_lines:
            body_elements.extend(periph_lines)
        body_elements.append("📡 무선상태 : Wi-Fi · BT On")

        tooltip_body = "\n".join(body_elements)
        self.indicator.set_tooltip_full(icon_name, tooltip_title, tooltip_body)

    def update_state_ui(self):
        self.updating_ui = True
        try:
            mode = self.get_current_mode()
            bat_info = self.get_battery_info()

            if mode in self.items:
                self.items[mode].set_active(True)

            self.update_icon_label_and_tooltip(mode, bat_info)

            status = bat_info['status']
            charge_limit = bat_info['charge_limit']
            status_ko = "충전 중" if status == 'Charging' else ("배터리 사용 중" if status == 'Discharging' else "충전 완료/대기")
            time_txt = f" ({bat_info['time_str']})" if bat_info['time_str'] else ""
            self.header_battery.set_label(f"🔋 배터리: {bat_info['capacity']}% - {status_ko}{time_txt}")

            if status == 'Charging':
                self.header_power.set_label(f"⚡ 현재 충전량: +{bat_info['power_w']:.2f} W (어댑터 충전 중)")
            elif status == 'Full' or (status == 'Not charging' and bat_info['capacity'] >= charge_limit):
                self.header_power.set_label(f"🔌 외부 AC 전원 연결됨 (보호 한도 {charge_limit}% 충전 대기)")
            else:
                self.header_power.set_label(f"⚡ 현재 사용량: -{bat_info['power_w']:.2f} W (배터리 사용 중)")

            self.header_protect.set_label(f"🛡️ 충전 보호: {bat_info['protect_str']}")

            peripherals = bat_info['peripherals']
            if peripherals:
                bt_strs = [f"{p['icon']} {p['name']}: {p['pct']}%" for p in peripherals]
                self.header_bt.set_label(" · ".join(bt_strs))
            else:
                self.header_bt.set_label("🎧 연결된 무선 기기 없음")
        finally:
            self.updating_ui = False

    def check_state_sync(self):
        self.update_state_ui()
        return True

    def on_show_detailed_status(self, widget):
        try:
            status_txt = subprocess.check_output(
                [get_manager_bin(), 'status'],
                stderr=subprocess.STDOUT
            ).decode('utf-8')
        except Exception as e:
            status_txt = str(e)

        self.notify_user('전원 관리 상세 정보', status_txt, 'dialog-information')

def main():
    app = PowerTrayApp()
    Gtk.main()

if __name__ == '__main__':
    main()
