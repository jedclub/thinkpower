#!/usr/bin/env bash
set -e

# ==============================================================================
# ThinkPower Uninstaller
# ==============================================================================

echo "========================================================="
echo "   🗑️ ThinkPower Uninstaller"
echo "========================================================="

echo "▶ [1/4] systemd 사용자 서비스 중지 및 비활성화..."
systemctl --user stop power-tray.service 2>/dev/null || true
systemctl --user disable power-tray.service 2>/dev/null || true
rm -f "$HOME/.config/systemd/user/power-tray.service"
systemctl --user daemon-reload

echo "▶ [2/4] 바이너리 파일 제거..."
rm -f "$HOME/.local/bin/power-tray" "$HOME/.local/bin/power-tray.py"
sudo rm -f "/usr/local/bin/power-profile-manager" "/usr/local/bin/power-tray"

echo "▶ [3/4] sudoers 권한 파일 제거..."
sudo rm -f "/etc/sudoers.d/99-power-profile-manager"

echo "▶ [4/4] 자동 시작 및 데스크톱 파일 제거..."
rm -f "$HOME/.config/autostart/power-tray.desktop"
rm -f "$HOME/.local/share/applications/power-ultra.desktop"
rm -f "$HOME/.cache/power_profile_mode" "$HOME/.cache/prev_kbd_backlight"

echo ""
echo "========================================================="
echo "  ✅ ThinkPower 제거가 완료되었습니다."
echo "========================================================="
