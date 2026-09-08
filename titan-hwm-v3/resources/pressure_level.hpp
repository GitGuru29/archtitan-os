// =============================================================================
// titan-hwm-v3/resources/pressure_level.hpp
// Phase 11 — Pressure Level Engine
//
// Aggregates /proc/meminfo + PSI memory pressure into a single PressureLevel.
// PSI is preferred (kernel 4.20+); falls back to RAM-only if unavailable.
// =============================================================================
#pragma once

#include "../core/types.hpp"
#include <string>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// MemInfo — snapshot of /proc/meminfo (in kB)
// ─────────────────────────────────────────────────────────────────────────────
struct MemInfo {
    long total_kb     = 0;
    long available_kb = 0;
    long swap_total_kb = 0;
    long swap_free_kb  = 0;

    float used_pct() const {
        if (total_kb <= 0) return 0.0f;
        return 100.0f * (1.0f - static_cast<float>(available_kb) /
                                 static_cast<float>(total_kb));
    }
    float free_pct() const { return 100.0f - used_pct(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// PSIStats — /proc/pressure/memory (avg10 only; sufficient for policy decisions)
// ─────────────────────────────────────────────────────────────────────────────
struct PSIStats {
    bool  valid      = false;
    float some_avg10 = 0.0f; // % of time at least one task stalled on memory
    float full_avg10 = 0.0f; // % of time ALL tasks stalled on memory
};

// ─────────────────────────────────────────────────────────────────────────────
// Readers
// ─────────────────────────────────────────────────────────────────────────────
MemInfo  read_meminfo();
PSIStats read_memory_psi();

// ─────────────────────────────────────────────────────────────────────────────
// compute_pressure — derive PressureLevel from combined signals
//
// Priority order:
//   1. RAM free < 15%                        → CRITICAL  (fast-path)
//   2. PSI full_avg10 >= 15% OR some >= 40%  → CRITICAL
//   3. PSI some_avg10 >= 20% OR RAM free <25%→ HIGH
//   4. PSI some_avg10 >= 5%  OR RAM free <50%→ MODERATE
//   5. Otherwise                             → NORMAL
// ─────────────────────────────────────────────────────────────────────────────
PressureLevel compute_pressure(const MemInfo& mem, const PSIStats& psi);

} // namespace thm
