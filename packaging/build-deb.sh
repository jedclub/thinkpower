#!/usr/bin/env bash
set -e

VERSION="${1:-1.0.0}"
ARCH="${2:-amd64}"
PKG_NAME="thinkpower"
STAGE_DIR="build/deb/${PKG_NAME}_${VERSION}_${ARCH}"
OUT_DIR="release"
DEB_FILE="${OUT_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"

echo "==> Staging Debian/Ubuntu package for ${PKG_NAME} ${VERSION} (${ARCH})..."
rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/DEBIAN"
mkdir -p "$STAGE_DIR/usr/bin"
mkdir -p "$STAGE_DIR/usr/lib/systemd/user"
mkdir -p "$STAGE_DIR/etc/sudoers.d"
mkdir -p "$STAGE_DIR/etc/xdg/autostart"
mkdir -p "$STAGE_DIR/usr/share/applications"
mkdir -p "$STAGE_DIR/usr/share/doc/${PKG_NAME}"
mkdir -p "$STAGE_DIR/usr/share/licenses/${PKG_NAME}"
mkdir -p "$OUT_DIR"

# 1. Install files
mkdir -p "$STAGE_DIR/lib/udev/rules.d"
install -m 755 bin/power-tray "$STAGE_DIR/usr/bin/power-tray"
install -m 755 src/power-profile-manager "$STAGE_DIR/usr/bin/power-profile-manager"
ln -sf power-profile-manager "$STAGE_DIR/usr/bin/tp-ppm"
install -m 440 src/config/99-power-profile-manager "$STAGE_DIR/etc/sudoers.d/99-power-profile-manager"
install -m 644 src/config/98-thinkpower-ac.rules "$STAGE_DIR/lib/udev/rules.d/98-thinkpower-ac.rules"
install -m 644 packaging/power-tray.service "$STAGE_DIR/usr/lib/systemd/user/power-tray.service"
install -m 644 packaging/power-tray.desktop "$STAGE_DIR/etc/xdg/autostart/power-tray.desktop"
install -m 644 packaging/power-ultra.desktop "$STAGE_DIR/usr/share/applications/power-ultra.desktop"
install -m 644 packaging/power-restore.desktop "$STAGE_DIR/usr/share/applications/power-restore.desktop"
install -m 644 LICENSE "$STAGE_DIR/usr/share/licenses/${PKG_NAME}/LICENSE"
install -m 644 README.md "$STAGE_DIR/usr/share/doc/${PKG_NAME}/README.md"

# 2. Control file
cat <<EOF > "$STAGE_DIR/DEBIAN/control"
Package: ${PKG_NAME}
Version: ${VERSION}
Section: utils
Priority: optional
Architecture: ${ARCH}
Maintainer: ThinkPower Contributors <https://github.com/jedclub/thinkpower>
Depends: libayatana-appindicator3-1, libgtk-3-0, libglib2.0-0, power-profiles-daemon, sudo
Recommends: kscreen-doctor, plasma-systemmonitor
Description: Advanced Battery & 4-Stage Power Management Tray for ThinkPad
 ThinkPower is a high-performance battery protection and 4-stage power
 management indicator for ThinkPad laptops running KDE Plasma and Linux.
 Features SIMD AVX2 filtering, battery charge threshold control,
 and 11-language localization.
EOF

# 3. Maintainer scripts
cat <<'EOF' > "$STAGE_DIR/DEBIAN/postinst"
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    chmod 0440 /etc/sudoers.d/99-power-profile-manager 2>/dev/null || true
    if command -v systemctl >/dev/null 2>&1; then
        echo "========================================================="
        echo "  ThinkPower installed successfully."
        echo "  Start user service with:"
        echo "    systemctl --user enable --now power-tray.service"
        echo "========================================================="
    fi
fi
exit 0
EOF
chmod 755 "$STAGE_DIR/DEBIAN/postinst"

cat <<'EOF' > "$STAGE_DIR/DEBIAN/postrm"
#!/bin/sh
set -e
if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    rm -f /etc/sudoers.d/99-power-profile-manager
fi
exit 0
EOF
chmod 755 "$STAGE_DIR/DEBIAN/postrm"

# 4. Build .deb
if command -v dpkg-deb >/dev/null 2>&1; then
    echo "==> Building .deb using dpkg-deb..."
    dpkg-deb --build --root-owner-group "$STAGE_DIR" "$DEB_FILE"
else
    echo "==> Building .deb using portable ar/tar..."
    TMP_BUILD="$(mktemp -d)"
    echo "2.0" > "$TMP_BUILD/debian-binary"
    
    tar -czf "$TMP_BUILD/control.tar.gz" --owner=0 --group=0 -C "$STAGE_DIR/DEBIAN" .
    tar -czf "$TMP_BUILD/data.tar.gz" --owner=0 --group=0 --exclude="./DEBIAN" -C "$STAGE_DIR" .
    
    (cd "$TMP_BUILD" && ar rcs "$OLDPWD/$DEB_FILE" debian-binary control.tar.gz data.tar.gz)
    rm -rf "$TMP_BUILD"
fi

rm -rf "build/deb"
echo "==> Debian package generated: $DEB_FILE"
