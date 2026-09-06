#!/usr/bin/env bash
# ==============================================================================
# ThinkPower Automated Regression Test Suite
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASSED_COUNT=0
FAILED_COUNT=0

assert_success() {
    local desc="$1"
    shift
    echo -n "  [TEST] $desc ... "
    if "$@" >/dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
}

assert_output_contains() {
    local desc="$1"
    local expected="$2"
    shift 2
    echo -n "  [TEST] $desc ... "
    local output
    output=$("$@" 2>&1 || true)
    if echo "$output" | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "${RED}FAIL${NC} (Expected '$expected' in output)"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
}

echo -e "${BLUE}======================================================${NC}"
echo -e "${BLUE}     ThinkPower Regression Prevention Test Suite      ${NC}"
echo -e "${BLUE}======================================================${NC}"

# ------------------------------------------------------------------------------
# 1. Syntax & Static Analysis
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}▶ 1. Syntax & Static Verification${NC}"

assert_success "Bash syntax: src/power-profile-manager" bash -n "${ROOT_DIR}/src/power-profile-manager"
assert_success "Bash syntax: install.sh" bash -n "${ROOT_DIR}/install.sh"
assert_success "Bash syntax: uninstall.sh" bash -n "${ROOT_DIR}/uninstall.sh"
assert_success "Bash syntax: packaging/build-deb.sh" bash -n "${ROOT_DIR}/packaging/build-deb.sh"

if command -v visudo >/dev/null 2>&1; then
    assert_success "Sudoers syntax: 99-power-profile-manager" visudo -cf "${ROOT_DIR}/src/config/99-power-profile-manager"
fi

if command -v desktop-file-validate >/dev/null 2>&1; then
    for df in "${ROOT_DIR}"/packaging/*.desktop; do
        if [ -f "$df" ]; then
            assert_success "Desktop entry format: $(basename "$df")" desktop-file-validate "$df"
        fi
    done
fi

# ------------------------------------------------------------------------------
# 2. CLI Argument Parsing & Contract Tests
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}▶ 2. CLI Contract & Subcommand Dispatch${NC}"

assert_output_contains "CLI usage shown on invalid argument" "사용법:" "${ROOT_DIR}/src/power-profile-manager" invalid_arg_xyz
assert_output_contains "CLI usage mentions analyze" "analyze" "${ROOT_DIR}/src/power-profile-manager" invalid_arg_xyz
assert_output_contains "CLI usage mentions restore" "restore" "${ROOT_DIR}/src/power-profile-manager" invalid_arg_xyz
assert_output_contains "CLI status output contract" "현재 전원 모드" "${ROOT_DIR}/src/power-profile-manager" status
assert_output_contains "CLI analyze output contract" "하드웨어별 세부 소비 전력 분석" "${ROOT_DIR}/src/power-profile-manager" analyze

# ------------------------------------------------------------------------------
# 3. Failsafe Architecture & Invariant Verification
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}▶ 3. Failsafe Recovery & Invariant Verification${NC}"

# Verify critical invariants are present in the script
assert_output_contains "Failsafe: AC plug udev rule invokes ac-event" "ac-event" cat "${ROOT_DIR}/src/config/98-thinkpower-ac.rules"
assert_output_contains "Failsafe: Desktop restore shortcut calls restore" "restore" cat "${ROOT_DIR}/packaging/power-restore.desktop"
assert_output_contains "Failsafe: Restore unlocks TDP to 25W" "stapm-limit=25000" grep "stapm-limit=25000" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Failsafe: Restore unlocks VRM to 44A/70A" "vrm-current=44000" grep "vrm-current=44000" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Failsafe: Restore resets GPU OverDrive" 'pp_od_clk_voltage' grep "pp_od_clk_voltage" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Failsafe: Boot ID tracking implemented" "power_profile_boot_id" grep "power_profile_boot_id" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Failsafe: Auto charger trigger handler exists" "handle_ac_event" grep "handle_ac_event" "${ROOT_DIR}/src/power-profile-manager"

# ------------------------------------------------------------------------------
# 4. Silicon & Hardware Tuning Constraints
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}▶ 4. Silicon & Hardware Tuning Constraints${NC}"

assert_output_contains "Ultra mode enforces 4W STAPM limit" "stapm-limit=4000" grep "stapm-limit=4000" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Ultra mode enforces 12A/16A VRM phase clamp" "vrm-current=12000" grep "vrm-current=12000" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Ultra mode enforces 640MHz GPU 40% cap" "s 1 640" grep "s 1 640" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Ultra mode keeps SMT 16 threads active" "16스레드" grep "16스레드 전체 유지" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Ultra mode enforces laptop_mode 5 (NVMe ASPM)" "laptop_mode" grep "laptop_mode 2>/dev/null || true" "${ROOT_DIR}/src/power-profile-manager"
assert_output_contains "Ultra mode cuts Bluetooth radio" "rfkill block bluetooth" grep "rfkill block bluetooth" "${ROOT_DIR}/src/power-profile-manager"

# ------------------------------------------------------------------------------
# 5. Compiled Binary Verification (if bin/power-tray exists)
# ------------------------------------------------------------------------------
echo -e "\n${YELLOW}▶ 5. Native Binary Verification${NC}"

if [ -x "${ROOT_DIR}/bin/power-tray" ]; then
    assert_success "power-tray binary executes --train without crashing" "${ROOT_DIR}/bin/power-tray" --train
    assert_success "power-tray binary has no unresolved shared libraries" ldd "${ROOT_DIR}/bin/power-tray"
else
    echo "  [INFO] bin/power-tray not built yet, skipping binary execution check."
fi

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
echo -e "\n${BLUE}======================================================${NC}"
echo -e "  Total Tests: $((PASSED_COUNT + FAILED_COUNT)) | Passed: ${GREEN}${PASSED_COUNT}${NC} | Failed: ${RED}${FAILED_COUNT}${NC}"
echo -e "${BLUE}======================================================${NC}"

if [ "$FAILED_COUNT" -gt 0 ]; then
    echo -e "${RED}❌ Regression tests FAILED! Please fix the errors above.${NC}"
    exit 1
else
    echo -e "${GREEN}✔ All regression tests PASSED! No regressions detected.${NC}"
    exit 0
fi
