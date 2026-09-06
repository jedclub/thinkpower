#!/usr/bin/env bash
set -e

# ==============================================================================
# ThinkPower Installer
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$HOME/.local/bin"
SYSTEMD_USER_DIR="$HOME/.config/systemd/user"
AUTOSTART_DIR="$HOME/.config/autostart"
APPS_DIR="$HOME/.local/share/applications"

echo "========================================================="
echo "   🚀 ThinkPower Installer (AMD / ThinkPad / KDE Plasma)"
echo "========================================================="

# 1. 디렉토리 준비
mkdir -p "$BIN_DIR" "$SYSTEMD_USER_DIR" "$AUTOSTART_DIR" "$APPS_DIR" "$HOME/.cache"

# 2. C++ 네이티브 바이너리 빌드 및 설치 (PGO + AVX2 + LTO 최적화)
echo "▶ [1/5] C++ PGO + AVX2 극한 최적화 빌드 진행 중..."
if make -C "$SCRIPT_DIR" pgo; then
    echo "  -> PGO 최적화 빌드 완료"
else
    echo "  -> 기본 LTO 네이티브 빌드로 대체"
    make -C "$SCRIPT_DIR" -j$(nproc)
fi
install -m 755 "$SCRIPT_DIR/bin/power-tray" "$BIN_DIR/power-tray"
install -m 755 "$SCRIPT_DIR/src/power-profile-manager" "$BIN_DIR/power-profile-manager"
ln -sf power-profile-manager "$BIN_DIR/tp-ppm"

echo "▶ [2/5] 루트 하드웨어 관리자 설치 중 (/usr/local/bin)..."
sudo cp "$SCRIPT_DIR/src/power-profile-manager" "/usr/local/bin/power-profile-manager"
sudo chmod +x "/usr/local/bin/power-profile-manager"
sudo ln -sf power-profile-manager "/usr/local/bin/tp-ppm" 2>/dev/null || true

# 3. sudoers 무암호 권한 및 Wi-Fi 절전 커널 모듈 등록
echo "▶ [3/5] sudoers 및 커널 모듈 최적화 설정 등록..."
sudo cp "$SCRIPT_DIR/src/config/99-power-profile-manager" "/etc/sudoers.d/99-power-profile-manager"
sudo chmod 0440 "/etc/sudoers.d/99-power-profile-manager"
if [ -f "$SCRIPT_DIR/src/config/thinkpower-wifi.conf" ]; then
    sudo cp "$SCRIPT_DIR/src/config/thinkpower-wifi.conf" "/etc/modprobe.d/thinkpower-wifi.conf" 2>/dev/null || true
fi

# 4. 데스크톱 및 시작 프로그램 등록
echo "▶ [4/5] KDE 데스크톱 및 트레이 자동 시작 등록..."
cp "$SCRIPT_DIR/src/config/power-tray.desktop" "$AUTOSTART_DIR/power-tray.desktop"
cp "$SCRIPT_DIR/src/config/power-ultra.desktop" "$APPS_DIR/power-ultra.desktop"
update-desktop-database "$APPS_DIR" 2>/dev/null || true

# 5. systemd 사용자 서비스 등록 및 실행
echo "▶ [5/5] systemd 사용자 서비스 활성화..."
cp "$SCRIPT_DIR/src/config/power-tray.service" "$SYSTEMD_USER_DIR/power-tray.service"
systemctl --user daemon-reload
systemctl --user enable --now power-tray.service

echo ""
echo "========================================================="
echo "  ✅ ThinkPower 설치가 성공적으로 완료되었습니다!"
echo "  작업표시줄(시스템 트레이)에서 실시간 배터리 및 전원 모드를"
echo "  바로 확인하실 수 있습니다."
echo "========================================================="
