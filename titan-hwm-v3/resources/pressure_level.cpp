// =============================================================================
// titan-hwm-v3/resources/pressure_level.cpp
// Phase 11 — implementation
// =============================================================================
#include "pressure_level.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// read_meminfo — parse /proc/meminfo
// ─────────────────────────────────────────────────────────────────────────────
MemInfo read_meminfo() {
    MemInfo m;
    std::ifstream f("/proc/meminfo");
    if (!f.is_open()) return m;
    std::string key;
    long val = 0;
    while (f >> key >> val) {
        if      (key == "MemTotal:")     m.total_kb      = val;
        else if (key == "MemAvailable:") m.available_kb  = val;
        else if (key == "SwapTotal:")    m.swap_total_kb = val;
        else if (key == "SwapFree:")     m.swap_free_kb  = val;
        f.ignore(256, '\n'); // skip rest of line (e.g. "kB")
    }
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// read_memory_psi — parse /proc/pressure/memory
// Format: "some avg10=X.XX avg60=Y.YY avg300=Z.ZZ total=N"
//         "full avg10=X.XX avg60=Y.YY avg300=Z.ZZ total=N"
// ─────────────────────────────────────────────────────────────────────────────
PSIStats read_memory_psi() {
    PSIStats p;
    std::ifstream f("/proc/pressure/memory");
    if (!f.is_open()) return p; // PSI not available (kernel < 4.20)

    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string kind;
        ss >> kind;

        float avg10 = 0.0f;
        std::string tok;
        while (ss >> tok) {
            if (tok.rfind("avg10=", 0) == 0) {
                try { avg10 = std::stof(tok.substr(6)); } catch (...) {}
                break;
            }
        }

        if (kind == "some") p.some_avg10 = avg10;
        if (kind == "full") p.full_avg10 = avg10;
    }
    p.valid = true;
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// compute_pressure — aggregate MemInfo + PSI → PressureLevel
// ─────────────────────────────────────────────────────────────────────────────
PressureLevel compute_pressure(const MemInfo& mem, const PSIStats& psi) {
    const float free_pct = mem.free_pct();

    // Fast-path: critically low RAM regardless of PSI availability
    if (free_pct < 15.0f) return PressureLevel::CRITICAL;

    if (psi.valid) {
        // PSI-based decisions (preferred — more precise than raw RAM %)
        if (psi.full_avg10 >= 15.0f || psi.some_avg10 >= 40.0f)
            return PressureLevel::CRITICAL;
        if (psi.some_avg10 >= 20.0f || free_pct < 25.0f)
            return PressureLevel::HIGH;
        if (psi.some_avg10 >= 5.0f  || free_pct < 50.0f)
            return PressureLevel::MODERATE;
        return PressureLevel::NORMAL;
    }

    // PSI unavailable — RAM-only fallback
    if (free_pct < 25.0f) return PressureLevel::CRITICAL;
    if (free_pct < 50.0f) return PressureLevel::HIGH;
    return PressureLevel::NORMAL;
}

} // namespace thm
