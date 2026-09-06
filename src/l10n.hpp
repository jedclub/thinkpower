#pragma once
#include <string>
#include <clocale>
#include <cctype>
#include <cstdlib>

namespace L10n {

enum class Lang {
    EN = 0, // Fallback default
    KO,
    JA,
    ZH_CN,
    ZH_TW,
    ES,
    DE,
    FR,
    RU,
    IT,
    PT,
    COUNT
};

enum class StrId {
    STATUS_CHARGING,
    STATUS_DISCHARGING,
    STATUS_FULL,
    STATUS_STANDBY,
    STATUS_COMPLETE_TAG,
    STATUS_CHARGE_LIMIT_FMT,
    STATUS_PENDING_AC,
    STATUS_PENDING_RESUME_FMT,

    TIME_HOURS_MINS_FMT,
    TIME_HOURS_FMT,
    TIME_MINS_FMT,
    TIME_LESS_THAN_MIN,
    TIME_REMAINING_PREFIX,
    TIME_TO_LIMIT_FMT,
    TIME_TO_FULL_PREFIX,
    TIME_REACHING_SOON_FMT,

    POWER_CHARGE_RATE_FMT,
    POWER_DISCHARGE_RATE_FMT,
    POWER_AC_DIRECT_FMT,
    POWER_AC_WAITING,

    PROTECT_TITLE,
    PROTECT_LIMIT_FMT,
    PROTECT_NORMAL,
    HEALTH_LINE_FMT,
    WIRELESS_STATUS,

    DEV_MOUSE,
    DEV_KEYBOARD,
    DEV_AUDIO,
    DEV_NONE,

    MENU_BATTERY_PREFIX,
    MENU_CURRENT_CHARGE_FMT,
    MENU_CURRENT_DISCHARGE_FMT,
    MENU_AC_MAINTAINING_FMT,
    MENU_AC_WAITING,

    MODE_TITLE_PERF,
    MODE_TITLE_BALANCED,
    MODE_TITLE_SAVE,
    MODE_TITLE_ULTRA,

    MODE_MENU_PERF,
    MODE_MENU_BALANCED,
    MODE_MENU_SAVE,
    MODE_MENU_ULTRA,

    MENU_SYSTEM_MONITOR,

    NOTIFY_PERF_TITLE,
    NOTIFY_PERF_MSG,
    NOTIFY_BAL_TITLE,
    NOTIFY_BAL_MSG,
    NOTIFY_SAVE_TITLE,
    NOTIFY_SAVE_MSG,
    NOTIFY_ULTRA_TITLE,
    NOTIFY_ULTRA_MSG,

    COUNT
};

// 11 languages x 49 strings translation table
static const char* const DICT[static_cast<size_t>(Lang::COUNT)][static_cast<size_t>(StrId::COUNT)] = {
    // -------------------------------------------------------------------------
    // EN: English (Default / Fallback)
    // -------------------------------------------------------------------------
    {
        "Charging",
        "On Battery",
        "Fully Charged (100%)",
        "Standby",
        "Full",
        "Charged (%d%% Limit)",
        "Waiting to Charge (AC connected)",
        "Waiting (resumes below %d%%)",

        "%dh %dm",
        "%dh",
        "%dm",
        "< 1m",
        "⏳ Remaining: ~",
        "until %d%%: ~",
        "to full: ~",
        "Reaching %d%% shortly",

        "⚡ Charge Rate: +%.1fW",
        "⚡ Power Draw: -%.1fW",
        "🔌 Power: AC Direct (%d%% Limit Kept)",
        "🔌 Power: AC Connected (Waiting)",

        "🛡️ Battery Protection: ",
        "%d%% (Lifespan)",
        "100% (Standard)",
        "🩺 Battery Health: %.1f%% (%d cycles)",
        "📡 Wireless: Wi-Fi · BT On",

        "Wireless Mouse",
        "Wireless Keyboard",
        "Bluetooth Audio",
        "🎧 No wireless devices connected",

        "🔋 Battery: ",
        "⚡ Current Charge: +%.2f W (Charging)",
        "⚡ Current Draw: -%.2f W (On Battery)",
        "🔌 External AC Connected (%d%% Limit Kept)",
        "🔌 External AC Connected (Waiting)",

        "⚡ Performance (4.1GHz)",
        "⚖️ Balanced (Dynamic Clock)",
        "🍃 Smart Save (1.7GHz)",
        "🛡️ Ultra Save (1.4GHz, 48Hz)",

        "⚡ Performance (4.1GHz Boost)",
        "⚖️ Balanced (Standard Dynamic Clock)",
        "🍃 Smart Save (1.7GHz Cap, 16T Active)",
        "🛡️ Ultra Save (1.4GHz, 48Hz, Max Savings)",

        "📈 Open System Monitor (KDE Performance)",

        "Power Profile: [Performance]",
        "System: Performance Mode | 4.1GHz CPU Boost Enabled",
        "Power Profile: [Balanced]",
        "System: Balanced Mode | Standard Dynamic Clock",
        "Power Profile: [Smart Save]",
        "System: Power Saver | 1.7GHz Cap & Panel Optimization\n(Wi-Fi, BT & 16 Threads 100% Active)",
        "Power Profile: [⚡ Ultra Save ON]",
        "System: Ultra Save | 1.4GHz Locked | 48Hz Panel | Full Savings\n(※ Wi-Fi and Bluetooth remain 100% connected)"
    },

    // -------------------------------------------------------------------------
    // KO: Korean (한국어)
    // -------------------------------------------------------------------------
    {
        "충전 중",
        "배터리 사용 중",
        "완충 (100%)",
        "대기",
        "완료",
        "충전 완료 (%d%% 보호 한도)",
        "충전 대기 (전원 연결됨)",
        "충전 대기 (%d%% 이하 재개)",

        "%d시간 %d분",
        "%d시간",
        "%d분",
        "1분 미만",
        "⏳ 남은시간 : 약 ",
        "%d%%까지 약 ",
        "완충까지 약 ",
        "%d%% 도달 직전",

        "⚡ 충전량 : +%.1fW",
        "⚡ 사용량 : -%.1fW",
        "🔌 전원 : 어댑터 직결 (%d%% 보호 한도 유지)",
        "🔌 전원 : 어댑터 연결됨 (충전 대기 중)",

        "🛡️ 보호한도 : ",
        "%d%% (수명 보호)",
        "100% (일반)",
        "🩺 배터리건강 : %.1f%% (%d회)",
        "📡 무선상태 : Wi-Fi · BT On",

        "무선 마우스",
        "무선 키보드",
        "블루투스 음향기기",
        "🎧 연결된 무선 기기 없음",

        "🔋 배터리: ",
        "⚡ 현재 충전량: +%.2f W (어댑터 충전 중)",
        "⚡ 현재 사용량: -%.2f W (배터리 사용 중)",
        "🔌 외부 AC 전원 연결됨 (보호 한도 %d%% 유지 중)",
        "🔌 외부 AC 전원 연결됨 (충전 대기 중)",

        "⚡ 성능 (4.1GHz)",
        "⚖️ 균형 (동적클럭)",
        "🍃 스마트 절전 (1.7GHz)",
        "🛡️ 극한 초절전 (1.4GHz, 48Hz)",

        "⚡ 성능 (4.1GHz 부스트)",
        "⚖️ 균형 (표준 동적 클럭)",
        "🍃 스마트 절전 (1.7GHz 상한, 16T 유지)",
        "🛡️ 극한 초절전 (1.4GHz, 48Hz, 백라이트MAX)",

        "📈 시스템 모니터 열기 (KDE 성능)",

        "전원 프로파일: [성능]",
        "시스템: 성능 모드 | 4.1GHz CPU 부스트 활성화",
        "전원 프로파일: [균형]",
        "시스템: 균형 모드 | 표준 동적 클럭",
        "전원 프로파일: [스마트 절전]",
        "시스템: 절전 모드 | 1.7GHz 상한 및 백라이트 최적화\n(Wi-Fi, BT, 16스레드 100% 정상 작동)",
        "전원 프로파일: [⚡ 극한 초절전 ON]",
        "시스템: 절전 모드 | 1.4GHz 고정 | 48Hz 다운클럭 | 백라이트MAX\n(※ Wi-Fi와 블루투스는 정상 유지됩니다)"
    },

    // -------------------------------------------------------------------------
    // JA: Japanese (日本語)
    // -------------------------------------------------------------------------
    {
        "充電中",
        "バッテリー駆動",
        "満充電 (100%)",
        "待機",
        "完了",
        "充電完了 (%d%% 保護上限)",
        "充電待機中 (AC接続済)",
        "充電待機中 (%d%%以下で再開)",

        "%d時間 %d分",
        "%d時間",
        "%d分",
        "1分未満",
        "⏳ 残り時間 : 約",
        "%d%%まで約",
        "満充電まで約",
        "%d%%到達目前",

        "⚡ 充電電力 : +%.1fW",
        "⚡ 消費電力 : -%.1fW",
        "🔌 電源 : AC直結 (%d%% 保護上限維持)",
        "🔌 電源 : AC接続済 (充電待機中)",

        "🛡️ バッテリー保護 : ",
        "%d%% (寿命保護)",
        "100% (通常)",
        "🩺 バッテリー健全度 : %.1f%% (%d回)",
        "📡 ワイヤレス : Wi-Fi · BT On",

        "ワイヤレスマウス",
        "ワイヤレスキーボード",
        "Bluetooth オーディオ",
        "🎧 接続中のワイヤレス機器なし",

        "🔋 バッテリー: ",
        "⚡ 現在の充電量: +%.2f W (充電中)",
        "⚡ 現在の消費量: -%.2f W (バッテリー使用中)",
        "🔌 外部AC電源接続済 (%d%% 保護維持)",
        "🔌 外部AC電源接続済 (充電待機中)",

        "⚡ パフォーマンス (4.1GHz)",
        "⚖️ バランス (動的クロック)",
        "🍃 スマート省電力 (1.7GHz)",
        "🛡️ 極限省電力 (1.4GHz, 48Hz)",

        "⚡ パフォーマンス (4.1GHz ブースト)",
        "⚖️ バランス (標準動的クロック)",
        "🍃 スマート省電力 (1.7GHz上限, 16T維持)",
        "🛡️ 極限省電力 (1.4GHz, 48Hz, 省電MAX)",

        "📈 システムモニターを開く (KDE性能)",

        "電源プロファイル: [パフォーマンス]",
        "システム: パフォーマンス | 4.1GHz CPUブースト有効",
        "電源プロファイル: [バランス]",
        "システム: バランス | 標準動的クロック",
        "電源プロファイル: [スマート省電力]",
        "システム: 省電力 | 1.7GHz上限および画面節電最適化\n(Wi-Fi, BT, 16スレッド 100%維持)",
        "電源プロファイル: [⚡ 極限省電力 ON]",
        "システム: 極限省電力 | 1.4GHz固定 | 48Hz | 画面節電MAX\n(※ Wi-FiとBluetoothは正常に維持されます)"
    },

    // -------------------------------------------------------------------------
    // ZH_CN: Simplified Chinese (简体中文)
    // -------------------------------------------------------------------------
    {
        "充电中",
        "电池供电",
        "已充满 (100%)",
        "待机",
        "完成",
        "充电完成 (%d%% 保护限额)",
        "等待充电 (已连接电源)",
        "等待充电 (低于%d%%恢复)",

        "%d小时 %d分",
        "%d小时",
        "%d分",
        "少于1分钟",
        "⏳ 剩余时间 : 约",
        "至%d%%约需",
        "充满电约需",
        "即将达到%d%%",

        "⚡ 充电功率 : +%.1fW",
        "⚡ 消耗功率 : -%.1fW",
        "🔌 电源 : 适配器直供 (保持%d%%保护限额)",
        "🔌 电源 : 已连接适配器 (等待充电)",

        "🛡️ 电池保护 : ",
        "%d%% (延长寿命)",
        "100% (标准)",
        "🩺 电池健康度 : %.1f%% (%d次循环)",
        "📡 无线状态 : Wi-Fi · BT 开启",

        "无线鼠标",
        "无线键盘",
        "蓝牙音频设备",
        "🎧 未连接无线设备",

        "🔋 电池: ",
        "⚡ 当前充电量: +%.2f W (适配器充电中)",
        "⚡ 当前消耗量: -%.2f W (电池供电中)",
        "🔌 外部AC电源已连接 (保持%d%%保护中)",
        "🔌 外部AC电源已连接 (等待充电)",

        "⚡ 高性能 (4.1GHz)",
        "⚖️ 平衡 (动态调频)",
        "🍃 智能省电 (1.7GHz)",
        "🛡️ 极限超省电 (1.4GHz, 48Hz)",

        "⚡ 高性能 (4.1GHz 睿频)",
        "⚖️ 平衡 (标准动态时钟)",
        "🍃 智能省电 (1.7GHz上限, 保持16线程)",
        "🛡️ 极限超省电 (1.4GHz, 48Hz, 极限节能)",

        "📈 打开系统监视器 (KDE性能)",

        "电源配置: [高性能]",
        "系统: 高性能模式 | 4.1GHz CPU睿频已启用",
        "电源配置: [平衡]",
        "系统: 平衡模式 | 标准动态调频",
        "电源配置: [智能省电]",
        "系统: 省电模式 | 1.7GHz上限及屏幕节电优化\n(Wi-Fi、蓝牙、16线程 100%正常工作)",
        "电源配置: [⚡ 极限超省电 开启]",
        "系统: 极限节能 | 1.4GHz锁定 | 48Hz刷新率 | 背光节电MAX\n(※ Wi-Fi与蓝牙保持100%连接)"
    },

    // -------------------------------------------------------------------------
    // ZH_TW: Traditional Chinese (繁體中文)
    // -------------------------------------------------------------------------
    {
        "充電中",
        "電池供電",
        "已充滿 (100%)",
        "待命",
        "完成",
        "充電完成 (%d%% 保護上限)",
        "等待充電 (已連接電源)",
        "等待充電 (低於%d%%恢復)",

        "%d小時 %d分",
        "%d小時",
        "%d分",
        "少於1分鐘",
        "⏳ 剩餘時間 : 約",
        "至%d%%約需",
        "充滿電約需",
        "即將達到%d%%",

        "⚡ 充電功率 : +%.1fW",
        "⚡ 消耗功率 : -%.1fW",
        "🔌 電源 : 變壓器直供 (維持%d%%保護上限)",
        "🔌 電源 : 已連接變壓器 (等待充電)",

        "🛡️ 電池保護 : ",
        "%d%% (延長壽命)",
        "100% (標準)",
        "🩺 電池健康度 : %.1f%% (%d次循環)",
        "📡 無線狀態 : Wi-Fi · BT 開啟",

        "無線滑鼠",
        "無線鍵盤",
        "藍牙音訊裝置",
        "🎧 未連接無線裝置",

        "🔋 電池: ",
        "⚡ 目前充電量: +%.2f W (充電中)",
        "⚡ 目前消耗量: -%.2f W (電池供電中)",
        "🔌 外部AC電源已連接 (維持%d%%保護中)",
        "🔌 外部AC電源已連接 (等待充電)",

        "⚡ 高效能 (4.1GHz)",
        "⚖️ 平衡 (動態時脈)",
        "🍃 智慧省電 (1.7GHz)",
        "🛡️ 極限超省電 (1.4GHz, 48Hz)",

        "⚡ 高效能 (4.1GHz 睿頻)",
        "⚖️ 平衡 (標準動態時脈)",
        "🍃 智慧省電 (1.7GHz上限, 維持16執行緒)",
        "🛡️ 極限超省電 (1.4GHz, 48Hz, 極致省電)",

        "📈 開啟系統監視器 (KDE效能)",

        "電源設定: [高效能]",
        "系統: 高效能模式 | 4.1GHz CPU睿頻已啟用",
        "電源設定: [平衡]",
        "系統: 平衡模式 | 標準動態時脈",
        "電源設定: [智慧省電]",
        "系統: 省電模式 | 1.7GHz上限及螢幕節電最佳化\n(Wi-Fi、藍牙、16執行緒 100%正常運作)",
        "電源設定: [⚡ 極限超省電 開啟]",
        "系統: 極限節能 | 1.4GHz鎖定 | 48Hz更新率 | 背光節電MAX\n(※ Wi-Fi與藍牙維持100%連線)"
    },

    // -------------------------------------------------------------------------
    // ES: Spanish (Español)
    // -------------------------------------------------------------------------
    {
        "Cargando",
        "Con batería",
        "Completamente cargado (100%)",
        "Espera",
        "Lleno",
        "Carga completa (Límite %d%%)",
        "Espera de carga (CA conectada)",
        "Espera (se reanuda bajo %d%%)",

        "%dh %dmin",
        "%dh",
        "%dmin",
        "< 1 min",
        "⏳ Restante: ~",
        "hasta %d%%: ~",
        "para carga completa: ~",
        "Alcanzando %d%% pronto",

        "⚡ Tasa de carga: +%.1fW",
        "⚡ Consumo: -%.1fW",
        "🔌 Energía: CA directa (Límite %d%% mantenido)",
        "🔌 Energía: CA conectada (En espera)",

        "🛡️ Protección de batería: ",
        "%d%% (Vida útil)",
        "100% (Estándar)",
        "🩺 Salud de batería: %.1f%% (%d ciclos)",
        "📡 Conexiones: Wi-Fi · BT On",

        "Ratón inalámbrico",
        "Teclado inalámbrico",
        "Audio Bluetooth",
        "🎧 Sin dispositivos inalámbricos",

        "🔋 Batería: ",
        "⚡ Carga actual: +%.2f W (Cargando)",
        "⚡ Consumo actual: -%.2f W (Con batería)",
        "🔌 CA externa conectada (Límite %d%%)",
        "🔌 CA externa conectada (En espera)",

        "⚡ Rendimiento (4.1GHz)",
        "⚖️ Equilibrado (Dinámico)",
        "🍃 Ahorro inteligente (1.7GHz)",
        "🛡️ Ultra ahorro (1.4GHz, 48Hz)",

        "⚡ Rendimiento (Turbo 4.1GHz)",
        "⚖️ Equilibrado (Reloj dinámico estándar)",
        "🍃 Ahorro inteligente (Máx 1.7GHz, 16T)",
        "🛡️ Ultra ahorro (1.4GHz, 48Hz, Ahorro máx)",

        "📈 Abrir monitor del sistema (KDE)",

        "Perfil de energía: [Rendimiento]",
        "Sistema: Modo rendimiento | Turbo CPU 4.1GHz activo",
        "Perfil de energía: [Equilibrado]",
        "Sistema: Modo equilibrado | Frecuencia dinámica estándar",
        "Perfil de energía: [Ahorro inteligente]",
        "Sistema: Ahorro de energía | Límite 1.7GHz y optimización de panel\n(Wi-Fi, BT y 16 hilos 100% activos)",
        "Perfil de energía: [⚡ Ultra ahorro ON]",
        "Sistema: Ultra ahorro | Frecuencia fija 1.4GHz | Panel 48Hz\n(※ Wi-Fi y Bluetooth permanecen 100% activos)"
    },

    // -------------------------------------------------------------------------
    // DE: German (Deutsch)
    // -------------------------------------------------------------------------
    {
        "Wird geladen",
        "Akkubetrieb",
        "Vollständig geladen (100%)",
        "Bereit",
        "Voll",
        "Geladen (%d%% Ladelimit)",
        "Ladebereitschaft (Netzteil angeschlossen)",
        "Ladebereitschaft (startet unter %d%%)",

        "%d Std. %d Min.",
        "%d Std.",
        "%d Min.",
        "< 1 Min.",
        "⏳ Verbleibend: ca. ",
        "bis %d%%: ca. ",
        "bis voll: ca. ",
        "Erreicht bald %d%%",

        "⚡ Laderate: +%.1fW",
        "⚡ Verbrauch: -%.1fW",
        "🔌 Strom: Netzbetrieb (%d%% Ladelimit aktiv)",
        "🔌 Strom: Netzteil angeschlossen (Warten)",

        "🛡️ Akkuschutz: ",
        "%d%% (Lebensdauer)",
        "100% (Standard)",
        "🩺 Akkuzustand: %.1f%% (%d Zyklen)",
        "📡 Funknetz: Wi-Fi · BT Ein",

        "Kabellose Maus",
        "Kabellose Tastatur",
        "Bluetooth-Audio",
        "🎧 Keine Funkgeräte verbunden",

        "🔋 Akku: ",
        "⚡ Aktuelle Ladung: +%.2f W (Wird geladen)",
        "⚡ Aktueller Verbrauch: -%.2f W (Akkubetrieb)",
        "🔌 Externes Netzteil verbunden (%d%% Ladelimit)",
        "🔌 Externes Netzteil verbunden (Warten)",

        "⚡ Leistung (4.1GHz)",
        "⚖️ Ausbalanciert (Dynamisch)",
        "🍃 Intelligentes Sparen (1.7GHz)",
        "🛡️ Ultra-Energiesparen (1.4GHz, 48Hz)",

        "⚡ Leistung (4.1GHz Boost)",
        "⚖️ Ausbalanciert (Standard-Dynamiktakt)",
        "🍃 Intelligentes Sparen (1.7GHz Limit, 16T aktiv)",
        "🛡️ Ultra-Energiesparen (1.4GHz, 48Hz, Max Sparen)",

        "📈 Systemmonitor öffnen (KDE-Leistung)",

        "Energieprofil: [Leistung]",
        "System: Leistungsmodus | 4.1GHz CPU-Boost aktiv",
        "Energieprofil: [Ausbalanciert]",
        "System: Ausbalancierter Modus | Standard-Dynamiktakt",
        "Energieprofil: [Intelligentes Sparen]",
        "System: Energiesparmodus | 1.7GHz Limit & Display-Optimierung\n(Wi-Fi, BT & 16 Threads 100% aktiv)",
        "Energieprofil: [⚡ Ultra-Energiesparen EIN]",
        "System: Ultra-Energiesparen | 1.4GHz fixiert | 48Hz Display\n(※ Wi-Fi und Bluetooth bleiben 100% aktiv)"
    },

    // -------------------------------------------------------------------------
    // FR: French (Français)
    // -------------------------------------------------------------------------
    {
        "En charge",
        "Sur batterie",
        "Complètement chargée (100%)",
        "Veille",
        "Plein",
        "Charge terminée (Limite %d%%)",
        "En attente de charge (Secteur branché)",
        "En attente (reprend sous %d%%)",

        "%dh %dmin",
        "%dh",
        "%dmin",
        "< 1 min",
        "⏳ Restant : ~",
        "jusqu'à %d%% : ~",
        "pour charger à 100% : ~",
        "Atteint bientôt %d%%",

        "⚡ Puissance de charge : +%.1fW",
        "⚡ Consommation : -%.1fW",
        "🔌 Alimentation : Secteur direct (Limite %d%% maintenue)",
        "🔌 Alimentation : Secteur connecté (En attente)",

        "🛡️ Protection batterie : ",
        "%d%% (Durée de vie)",
        "100% (Standard)",
        "🩺 Santé de la batterie : %.1f%% (%d cycles)",
        "📡 Sans fil : Wi-Fi · BT On",

        "Souris sans fil",
        "Clavier sans fil",
        "Audio Bluetooth",
        "🎧 Aucun appareil sans fil connecté",

        "🔋 Batterie : ",
        "⚡ Charge actuelle : +%.2f W (En charge)",
        "⚡ Consommation actuelle : -%.2f W (Sur batterie)",
        "🔌 Secteur externe connecté (Limite %d%%)",
        "🔌 Secteur externe connecté (En attente)",

        "⚡ Performance (4.1GHz)",
        "⚖️ Équilibré (Dynamique)",
        "🍃 Économie intelligente (1.7GHz)",
        "🛡️ Ultra économie (1.4GHz, 48Hz)",

        "⚡ Performance (Boost 4.1GHz)",
        "⚖️ Équilibré (Horloge dynamique standard)",
        "🍃 Économie intelligente (Max 1.7GHz, 16T)",
        "🛡️ Ultra économie (1.4GHz, 48Hz, Économie max)",

        "📈 Ouvrir le moniteur système (KDE)",

        "Profil d'énergie : [Performance]",
        "Système : Mode performance | Boost CPU 4.1GHz activé",
        "Profil d'énergie : [Équilibré]",
        "Système : Mode équilibré | Fréquence dynamique standard",
        "Profil d'énergie : [Économie intelligente]",
        "Système : Économie d'énergie | Max 1.7GHz et optimisation écran\n(Wi-Fi, BT et 16 threads 100% actifs)",
        "Profil d'énergie : [⚡ Ultra économie ACTIVÉE]",
        "Système : Ultra économie | Bloqué à 1.4GHz | Écran 48Hz\n(※ Wi-Fi et Bluetooth restent 100% connectés)"
    },

    // -------------------------------------------------------------------------
    // RU: Russian (Русский)
    // -------------------------------------------------------------------------
    {
        "Зарядка",
        "От батареи",
        "Полный заряд (100%)",
        "Ожидание",
        "Готово",
        "Зарядка завершена (Лимит %d%%)",
        "Ожидание зарядки (Сеть подключена)",
        "Ожидание (возобновится ниже %d%%)",

        "%d ч %d мин",
        "%d ч",
        "%d мин",
        "< 1 мин",
        "⏳ Осталось: ~",
        "до %d%%: ~",
        "до полной зарядки: ~",
        "Скоро достигнет %d%%",

        "⚡ Мощность зарядки: +%.1fW",
        "⚡ Потребление: -%.1fW",
        "🔌 Питание: Прямое от сети (Лимит %d%% активен)",
        "🔌 Питание: Сеть подключена (Ожидание)",

        "🛡️ Защита батареи: ",
        "%d%% (Продление службы)",
        "100% (Стандарт)",
        "🩺 Здоровье батареи: %.1f%% (%d циклов)",
        "📡 Беспроводная связь: Wi-Fi · BT Вкл",

        "Беспроводная мышь",
        "Беспроводная клавиатура",
        "Bluetooth аудио",
        "🎧 Нет подключенных беспроводных устройств",

        "🔋 Батарея: ",
        "⚡ Текущая зарядка: +%.2f W (Зарядка)",
        "⚡ Текущее потребление: -%.2f W (От батареи)",
        "🔌 Внешнее питание подключено (Лимит %d%%)",
        "🔌 Внешнее питание подключено (Ожидание)",

        "⚡ Производительность (4.1GHz)",
        "⚖️ Сбалансированный (Динамический)",
        "🍃 Умное энергосбережение (1.7GHz)",
        "🛡️ Экстремальное энергосбережение (1.4GHz, 48Hz)",

        "⚡ Производительность (4.1GHz Буст)",
        "⚖️ Сбалансированный (Стандартная частота)",
        "🍃 Умное энергосбережение (1.7GHz макс, 16T)",
        "🛡️ Экстремальное энергосбережение (1.4GHz, 48Hz)",

        "📈 Открыть системный монитор (KDE)",

        "Профиль питания: [Производительность]",
        "Система: Производительность | 4.1GHz CPU Boost активен",
        "Профиль питания: [Сбалансированный]",
        "Система: Сбалансированный режим | Стандартная частота",
        "Профиль питания: [Умное энергосбережение]",
        "Система: Энергосбережение | Лимит 1.7GHz и оптимизация экрана\n(Wi-Fi, BT и 16 потоков 100% активны)",
        "Профиль питания: [⚡ Экстремальное энергосбережение ВКЛ]",
        "Система: Экстремальное сбережение | 1.4GHz | 48Hz дисплей\n(※ Wi-Fi и Bluetooth остаются активными на 100%)"
    },

    // -------------------------------------------------------------------------
    // IT: Italian (Italiano)
    // -------------------------------------------------------------------------
    {
        "In carica",
        "A batteria",
        "Completamente carica (100%)",
        "Attesa",
        "Completo",
        "Carica completata (Limite %d%%)",
        "In attesa di carica (Alimentatore collegato)",
        "In attesa (riprende sotto %d%%)",

        "%dh %dmin",
        "%dh",
        "%dmin",
        "< 1 min",
        "⏳ Rimanente: ~",
        "fino al %d%%: ~",
        "alla carica completa: ~",
        "Quasi al %d%%",

        "⚡ Velocità di carica: +%.1fW",
        "⚡ Consumo: -%.1fW",
        "🔌 Alimentazione: Rete diretta (Limite %d%% mantenuto)",
        "🔌 Alimentazione: Rete connessa (In attesa)",

        "🛡️ Protezione batteria: ",
        "%d%% (Durata)",
        "100% (Standard)",
        "🩺 Stato batteria: %.1f%% (%d cicli)",
        "📡 Connessioni: Wi-Fi · BT On",

        "Mouse wireless",
        "Tastiera wireless",
        "Audio Bluetooth",
        "🎧 Nessun dispositivo wireless collegato",

        "🔋 Batteria: ",
        "⚡ Carica attuale: +%.2f W (In carica)",
        "⚡ Consumo attuale: -%.2f W (A batteria)",
        "🔌 Alimentazione CA collegata (Limite %d%%)",
        "🔌 Alimentazione CA collegata (In attesa)",

        "⚡ Prestazioni (4.1GHz)",
        "⚖️ Bilanciato (Dinamico)",
        "🍃 Risparmio intelligente (1.7GHz)",
        "🛡️ Ultra risparmio (1.4GHz, 48Hz)",

        "⚡ Prestazioni (Boost 4.1GHz)",
        "⚖️ Bilanciato (Clock dinamico standard)",
        "🍃 Risparmio intelligente (Max 1.7GHz, 16T)",
        "🛡️ Ultra risparmio (1.4GHz, 48Hz, Risparmio max)",

        "📈 Apri monitor di sistema (KDE)",

        "Profilo energetico: [Prestazioni]",
        "Sistema: Modalità prestazioni | Boost CPU 4.1GHz attivo",
        "Profilo energetico: [Bilanciato]",
        "Sistema: Modalità bilanciata | Clock dinamico standard",
        "Profilo energetico: [Risparmio intelligente]",
        "Sistema: Risparmio energetico | Limite 1.7GHz e ottimizzazione pannello\n(Wi-Fi, BT e 16 thread 100% attivi)",
        "Profilo energetico: [⚡ Ultra risparmio ATTIVATO]",
        "Sistema: Ultra risparmio | 1.4GHz bloccato | Schermo 48Hz\n(※ Wi-Fi e Bluetooth restano attivi al 100%)"
    },

    // -------------------------------------------------------------------------
    // PT: Portuguese (Português)
    // -------------------------------------------------------------------------
    {
        "Carregando",
        "Na bateria",
        "Totalmente carregada (100%)",
        "Espera",
        "Completo",
        "Carga concluída (Limite %d%%)",
        "Aguardando carga (CA conectada)",
        "Aguardando (retoma abaixo de %d%%)",

        "%dh %dmin",
        "%dh",
        "%dmin",
        "< 1 min",
        "⏳ Restante: ~",
        "até %d%%: ~",
        "até a carga total: ~",
        "Quase em %d%%",

        "⚡ Taxa de carga: +%.1fW",
        "⚡ Consumo: -%.1fW",
        "🔌 Energia: CA direta (Limite %d%% mantido)",
        "🔌 Energia: CA conectada (Aguardando)",

        "🛡️ Proteção da bateria: ",
        "%d%% (Vida útil)",
        "100% (Padrão)",
        "🩺 Saúde da bateria: %.1f%% (%d ciclos)",
        "📡 Sem fio: Wi-Fi · BT Ligado",

        "Mouse sem fio",
        "Teclado sem fio",
        "Áudio Bluetooth",
        "🎧 Nenhum dispositivo sem fio conectado",

        "🔋 Bateria: ",
        "⚡ Carga atual: +%.2f W (Carregando)",
        "⚡ Consumo atual: -%.2f W (Na bateria)",
        "🔌 CA externa conectada (Limite %d%% mantido)",
        "🔌 CA externa conectada (Aguardando)",

        "⚡ Desempenho (4.1GHz)",
        "⚖️ Equilibrado (Dinâmico)",
        "🍃 Economia inteligente (1.7GHz)",
        "🛡️ Ultra economia (1.4GHz, 48Hz)",

        "⚡ Desempenho (Boost 4.1GHz)",
        "⚖️ Equilibrado (Clock dinâmico padrão)",
        "🍃 Economia inteligente (Máx 1.7GHz, 16T)",
        "🛡️ Ultra economia (1.4GHz, 48Hz, Economia máx)",

        "📈 Abrir monitor do sistema (KDE)",

        "Perfil de energia: [Desempenho]",
        "Sistema: Modo desempenho | Boost de CPU 4.1GHz ativado",
        "Perfil de energia: [Equilibrado]",
        "Sistema: Modo equilibrado | Clock dinâmico padrão",
        "Perfil de energia: [Economia inteligente]",
        "Sistema: Economia de energia | Limite de 1.7GHz e otimização de tela\n(Wi-Fi, BT e 16 threads 100% ativos)",
        "Perfil de energia: [⚡ Ultra economia ATIVADA]",
        "Sistema: Ultra economia | Travado em 1.4GHz | Painel 48Hz\n(※ Wi-Fi e Bluetooth permanecem 100% conectados)"
    }
};

class LocaleManager {
public:
    static LocaleManager& instance() {
        static LocaleManager inst;
        return inst;
    }

    Lang get_lang() const { return lang; }

    const char* tr(StrId id) const {
        size_t l = static_cast<size_t>(lang);
        size_t s = static_cast<size_t>(id);
        if (l >= static_cast<size_t>(Lang::COUNT) || s >= static_cast<size_t>(StrId::COUNT)) {
            return DICT[0][s];
        }
        const char *val = DICT[l][s];
        return (val && *val) ? val : DICT[0][s];
    }

private:
    Lang lang = Lang::EN;

    LocaleManager() {
        lang = detect_lang();
    }

    static Lang detect_lang() {
        const char *env = getenv("LC_ALL");
        if (!env || !*env) env = getenv("LC_MESSAGES");
        if (!env || !*env) env = getenv("LANG");
        if (!env || !*env) {
            char *loc = setlocale(LC_MESSAGES, "");
            if (loc) env = loc;
        }
        if (!env || !*env) return Lang::EN;

        std::string s(env);
        for (char &c : s) c = std::tolower(c);

        if (s.rfind("ko", 0) == 0) return Lang::KO;
        if (s.rfind("ja", 0) == 0) return Lang::JA;
        if (s.find("zh_tw") != std::string::npos || s.find("zh_hk") != std::string::npos) return Lang::ZH_TW;
        if (s.rfind("zh", 0) == 0) return Lang::ZH_CN;
        if (s.rfind("es", 0) == 0) return Lang::ES;
        if (s.rfind("de", 0) == 0) return Lang::DE;
        if (s.rfind("fr", 0) == 0) return Lang::FR;
        if (s.rfind("ru", 0) == 0) return Lang::RU;
        if (s.rfind("it", 0) == 0) return Lang::IT;
        if (s.rfind("pt", 0) == 0) return Lang::PT;

        return Lang::EN;
    }
};

inline const char* tr(StrId id) {
    return LocaleManager::instance().tr(id);
}

inline bool is_korean() {
    return LocaleManager::instance().get_lang() == Lang::KO;
}

} // namespace L10n
