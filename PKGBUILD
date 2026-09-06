# Maintainer: ThinkPower Contributors
pkgname=thinkpower
pkgver=1.0.0
pkgrel=1
pkgdesc="Advanced Battery & 4-Stage Power Management Tray for ThinkPad (KDE Plasma, AMD Ryzen)"
arch=('x86_64')
url="https://github.com/thinkpower/thinkpower"
license=('GPL-3.0-or-later')
depends=('gtk3' 'libayatana-appindicator' 'glib2' 'power-profiles-daemon' 'sudo')
makedepends=('gcc' 'make' 'pkgconf')
optdepends=(
    'kscreen-doctor: Refresh rate switching (60Hz <-> 48Hz in Ultra mode)'
    'plasma-systemmonitor: Direct KDE System Monitor launcher'
    'baloo: Indexer suspend/resume in Ultra mode'
    'powertop: PCIe runtime power optimization'
    'ryzenadj: Direct hardware TDP management'
)
install=packaging/thinkpower.install
options=('!strip')

build() {
    cd "$startdir"
    make pgo
}

package() {
    cd "$startdir"

    # 1. Binaries
    install -Dm755 bin/power-tray "$pkgdir/usr/bin/power-tray"
    install -Dm755 src/power-profile-manager "$pkgdir/usr/bin/power-profile-manager"
    ln -sf power-profile-manager "$pkgdir/usr/bin/tp-ppm"

    # 2. Sudoers & Udev rules
    install -Dm440 src/config/99-power-profile-manager "$pkgdir/etc/sudoers.d/99-power-profile-manager"
    install -Dm644 src/config/98-thinkpower-ac.rules "$pkgdir/usr/lib/udev/rules.d/98-thinkpower-ac.rules"

    # 3. Systemd user service
    install -Dm644 packaging/power-tray.service "$pkgdir/usr/lib/systemd/user/power-tray.service"

    # 4. Desktop entries
    install -Dm644 packaging/power-tray.desktop "$pkgdir/etc/xdg/autostart/power-tray.desktop"
    install -Dm644 packaging/power-ultra.desktop "$pkgdir/usr/share/applications/power-ultra.desktop"
    install -Dm644 packaging/power-restore.desktop "$pkgdir/usr/share/applications/power-restore.desktop"

    # 5. Documentation & License
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
