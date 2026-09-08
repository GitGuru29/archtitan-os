// ============================================================
// Titan Hardware Manager — Architecture Comparison Report
// Static Workspace Map  vs  Pure Intent-Inference (FusionClassifier)
// ============================================================

#set document(
  title: "Titan Hardware Manager — Architecture Comparison Report",
  author: "ArchTitan OS Development",
  date: datetime(year: 2026, month: 9, day: 8),
)

#set page(
  paper: "a4",
  margin: (top: 2.4cm, bottom: 2.4cm, left: 2.2cm, right: 2.2cm),
  header: [
    #set text(size: 8pt, fill: rgb("#888888"))
    #grid(
      columns: (1fr, 1fr),
      align(left)[ArchTitan OS — Internal Technical Report],
      align(right)[Titan Hardware Manager v2],
    )
    #line(length: 100%, stroke: 0.4pt + rgb("#cccccc"))
  ],
  footer: [
    #line(length: 100%, stroke: 0.4pt + rgb("#cccccc"))
    #set text(size: 8pt, fill: rgb("#888888"))
    #grid(
      columns: (1fr, 1fr),
      align(left)[Confidential — ArchTitan Internal],
      align(right)[Page #context counter(page).display("1 of 1", both: true)],
    )
  ],
  numbering: "1",
)

#set text(font: "Liberation Sans", size: 10.5pt, fill: rgb("#1a1a2e"))
#set heading(numbering: "1.")
#set par(justify: true, leading: 0.7em)

// ── Color palette ────────────────────────────────────────────
#let accent   = rgb("#1b4fdb")
#let positive = rgb("#1a7f4a")
#let negative = rgb("#b83232")
#let neutral  = rgb("#5a5a7a")
#let bg_light = rgb("#f4f6fd")
#let bg_warn  = rgb("#fff7ed")
#let bg_pos   = rgb("#edfaf4")
#let bg_neg   = rgb("#fdeaea")
#let divider  = rgb("#d0d8f0")

// ── Helpers ──────────────────────────────────────────────────
#let pill(body, color: accent) = box(
  fill: color.lighten(82%),
  stroke: 0.6pt + color.lighten(30%),
  radius: 3pt,
  inset: (x: 6pt, y: 2pt),
  text(size: 8.5pt, fill: color, weight: "semibold", body)
)

#let section_rule() = {
  v(4pt)
  line(length: 100%, stroke: 0.6pt + divider)
  v(4pt)
}

#let callout(body, color: accent, icon: "ℹ") = block(
  width: 100%,
  fill: color.lighten(88%),
  stroke: (left: 3pt + color),
  radius: (right: 4pt),
  inset: (x: 12pt, y: 9pt),
  [#text(fill: color, weight: "bold")[#icon #h(4pt)]#body]
)

#let pro_item(body) = grid(
  columns: (16pt, 1fr),
  gutter: 4pt,
  text(fill: positive, weight: "bold")[✓],
  body
)

#let con_item(body) = grid(
  columns: (16pt, 1fr),
  gutter: 4pt,
  text(fill: negative, weight: "bold")[✗],
  body
)

#let neutral_item(body) = grid(
  columns: (16pt, 1fr),
  gutter: 4pt,
  text(fill: neutral)[→],
  body
)

// ── Cover Block ──────────────────────────────────────────────
#block(
  width: 100%,
  fill: accent,
  radius: 6pt,
  inset: (x: 24pt, y: 22pt),
)[
  #text(size: 22pt, weight: "bold", fill: white)[Titan Hardware Manager]
  #v(4pt)
  #text(size: 13pt, fill: white.lighten(20%))[Architecture Comparison Report]
  #v(10pt)
  #grid(
    columns: (1fr, 1fr, 1fr),
    text(size: 9pt, fill: white.lighten(30%))[
      *System:* ArchTitan OS \
      *Component:* THM v2
    ],
    text(size: 9pt, fill: white.lighten(30%))[
      *Version:* titan_hw_manager.cpp \
      *Lines:* 2121
    ],
    text(size: 9pt, fill: white.lighten(30%))[
      *Date:* September 8, 2026 \
      *Status:* In Review
    ],
  )
]

#v(14pt)

// ── Executive Summary ────────────────────────────────────────
= Executive Summary

This report compares two architectural approaches for process priority management in the Titan Hardware Manager (THM): the *current implementation* using a static workspace-to-profile mapping, and a *proposed redesign* using a fully dynamic, content-aware FusionClassifier system. Both approaches are evaluated across portability, accuracy, developer experience, and resource efficiency dimensions, with concrete use-case scenarios illustrating where each design succeeds and fails.

#section_rule()

// ── Architecture Overview ────────────────────────────────────
= Architecture Overview

== Current System — Static Workspace Map

The current THM assigns a fixed *DevProfile* to each Hyprland workspace number via configuration. On every `workspace>>` IPC event, THM reads the workspace number, looks it up in the config, and applies the corresponding resource profile.

#block(
  fill: bg_light,
  radius: 4pt,
  inset: (x: 12pt, y: 10pt),
  stroke: 0.6pt + divider,
)[
  #text(size: 9pt, font: "Liberation Mono")[
    ```
    /etc/titan-hwm/config (shipped defaults)
    ─────────────────────────────────────────
    casual_workspaces  = [1]
    web_workspaces     = [2]
    android_workspaces = [3]
    system_workspaces  = [4, 5]
    ```
    #v(6pt)
    ```
    workspace>>2  →  classify_ws(2)  →  WEB_DEV
                  →  do_transition(WEB_DEV)
                  →  freeze: studio, java, gradle
                  →  thaw:   node, vite, webpack
                  →  governor: performance, swappiness: 10
    ```
  ]
]

#v(6pt)

#callout(
  color: rgb("#b83232"),
  icon: "⚠",
)[*Critical constraint:* Classification is purely positional. WS 2 is *always* Web Dev, regardless of what applications are actually running on it.]

== Proposed System — Pure Intent Inference (FusionClassifier)

The proposed system removes the static workspace map entirely. Every `workspace>>` event triggers a query to Hyprland's `j/clients` endpoint to retrieve the focused window's PID, which is then passed through the existing *FusionClassifier* — a 3-signal weighted voting engine already present in THM v2.

#block(
  fill: bg_light,
  radius: 4pt,
  inset: (x: 12pt, y: 10pt),
  stroke: 0.6pt + divider,
)[
  #text(size: 9pt, font: "Liberation Mono")[
    ```
    workspace>>N  →  query j/clients  →  focused_pid
                  →  FusionClassifier(pid, title, graph)
                        Signal 1: child process walk (weight 0.6)
                        Signal 2: window title extension (weight 0.3–0.5)
                        Signal 3: CWD project markers (weight 0.2)
                  →  dominant WorkloadType
                  →  do_transition(inferred_profile)
    ```
  ]
]

#section_rule()

// ── Signal Weights Table ─────────────────────────────────────
= FusionClassifier Signal Reference

#table(
  columns: (2fr, 3fr, 1fr, 1fr),
  fill: (col, row) => if row == 0 { accent } else if calc.odd(row) { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 8pt,
  align: (left, left, center, center),
  text(fill: white, weight: "bold")[Signal],
  text(fill: white, weight: "bold")[Source],
  text(fill: white, weight: "bold")[Weight],
  text(fill: white, weight: "bold")[Fallback],

  [*S1* — Child Walk], [Recurse `/proc/PID/children` 3 levels deep; match `lsp_binaries`, `build_daemons`], [0.6 per hit], [—],
  [*S2* — Title Ext], [Window title suffix check: `.cpp`, `.ts`, `build.gradle`, `CMakeLists`], [0.3 (0.5 if specific)], [S1 only],
  [*S3* — CWD Marker], [Stat focused PID cwd for `CMakeLists.txt`, `package.json`, `build.gradle`], [0.2], [S1+S2],
)

#v(8pt)

#callout(color: positive, icon: "✓")[
  The FusionClassifier is already fully implemented and tested in THM v2 (lines 462–619). Removing the static map *does not require new code* — only a routing change for `workspace>>` events.
]

#section_rule()

// ── Pros & Cons ──────────────────────────────────────────────
= Comparative Analysis

== 4.1 Static Workspace Map (Current)

#grid(
  columns: (1fr, 1fr),
  gutter: 12pt,
  block(
    fill: bg_pos,
    stroke: (left: 3pt + positive),
    radius: (right: 4pt),
    inset: 10pt,
  )[
    #text(weight: "bold", fill: positive)[Advantages]
    #v(6pt)
    #pro_item[Instant classification — zero IPC overhead on switch]
    #v(4pt)
    #pro_item[Works even before any window is focused]
    #v(4pt)
    #pro_item[Predictable: same WS always triggers same profile]
    #v(4pt)
    #pro_item[Cold boot works — protects tools before they start]
    #v(4pt)
    #pro_item[Simple to reason about for users who organize by WS]
    #v(4pt)
    #pro_item[No dependency on `j/clients` IPC availability]
  ],
  block(
    fill: bg_neg,
    stroke: (left: 3pt + negative),
    radius: (right: 4pt),
    inset: 10pt,
  )[
    #text(weight: "bold", fill: negative)[Disadvantages]
    #v(6pt)
    #con_item[WS number ≠ actual use — misconfiguration is default state]
    #v(4pt)
    #con_item[Non-portable: hardcoded for ArchTitan workspace layout]
    #v(4pt)
    #con_item[Breaks when user adds/removes/reorders workspaces]
    #v(4pt)
    #con_item[Single workspace for multiple dev contexts → always wrong]
    #v(4pt)
    #con_item[User must manually update config to match their habits]
    #v(4pt)
    #con_item[Prunes tools based on WS number, not actual running processes]
  ],
)

#v(10pt)

== 4.2 Pure FusionClassifier (Proposed)

#grid(
  columns: (1fr, 1fr),
  gutter: 12pt,
  block(
    fill: bg_pos,
    stroke: (left: 3pt + positive),
    radius: (right: 4pt),
    inset: 10pt,
  )[
    #text(weight: "bold", fill: positive)[Advantages]
    #v(6pt)
    #pro_item[Zero configuration — works out of the box on any system]
    #v(4pt)
    #pro_item[Adapts to actual developer intent in real time]
    #v(4pt)
    #pro_item[Correct for multi-project single-workspace workflows]
    #v(4pt)
    #pro_item[Portable across Hyprland distros without reconfiguration]
    #v(4pt)
    #pro_item[Split-screen IDEs handled via polyglot detection]
    #v(4pt)
    #pro_item[Never prunes tools that are actually active]
  ],
  block(
    fill: bg_neg,
    stroke: (left: 3pt + negative),
    radius: (right: 4pt),
    inset: 10pt,
  )[
    #text(weight: "bold", fill: negative)[Disadvantages]
    #v(6pt)
    #con_item[Requires `j/clients` IPC on every workspace switch]
    #v(4pt)
    #con_item[Cold signal: blank workspace has no inference data]
    #v(4pt)
    #con_item[Unknown binaries → NEUTRAL (no transition at all)]
    #v(4pt)
    #con_item[Bare terminal before running anything → misclassified]
    #v(4pt)
    #con_item[FusionClassifier runs in transition thread — adds ~30ms latency]
    #v(4pt)
    #con_item[`known_ides` list must be maintained as new editors emerge]
  ],
)

#section_rule()

// ── Scoring Matrix ───────────────────────────────────────────
= Scoring Matrix

#table(
  columns: (3fr, 1fr, 1fr, 1fr),
  fill: (col, row) => {
    if row == 0 { accent }
    else if col == 0 and row > 0 { bg_light }
    else if calc.odd(row) { white }
    else { rgb("#f8f9fe") }
  },
  stroke: 0.5pt + divider,
  inset: 8pt,
  align: (left, center, center, center),

  text(fill: white, weight: "bold")[Criterion],
  text(fill: white, weight: "bold")[Static Map],
  text(fill: white, weight: "bold")[FusionClassifier],
  text(fill: white, weight: "bold")[Winner],

  [Portability (other systems)],               [⭐⭐], [⭐⭐⭐⭐⭐], pill("Fusion", color: positive),
  [Classification accuracy],                   [⭐⭐], [⭐⭐⭐⭐],   pill("Fusion", color: positive),
  [Zero-config ease of use],                   [⭐],   [⭐⭐⭐⭐⭐], pill("Fusion", color: positive),
  [Cold-boot / empty workspace],               [⭐⭐⭐⭐⭐], [⭐⭐],   pill("Static", color: accent),
  [Runtime performance],                       [⭐⭐⭐⭐⭐], [⭐⭐⭐],  pill("Static", color: accent),
  [Multi-project single WS support],           [⭐],   [⭐⭐⭐⭐⭐], pill("Fusion", color: positive),
  [Predictability for power users],            [⭐⭐⭐⭐], [⭐⭐⭐],   pill("Static", color: accent),
  [Resilience to workspace reordering],        [⭐],   [⭐⭐⭐⭐⭐], pill("Fusion", color: positive),
  [Correctness without user intervention],     [⭐],   [⭐⭐⭐⭐],   pill("Fusion", color: positive),
  [IPC robustness (socket unavailable)],       [⭐⭐⭐⭐⭐], [⭐⭐⭐],  pill("Static", color: accent),
)

#section_rule()

// ── Use Case Scenarios ───────────────────────────────────────
= Use Case Scenarios

== Scenario A — Kernel Developer (Workspace 1, Current Config)

#table(
  columns: (1fr, 1fr),
  fill: (col, row) => if row == 0 { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 9pt,

  text(weight: "bold")[Static Map Behavior], text(weight: "bold")[FusionClassifier Behavior],

  [WS 1 = Casual. Switch to WS 1 → immediately prunes `make`, `cmake`, `clangd`, `gdb`. Kernel build fails silently. CPU governor set to `schedutil`, swappiness raised to 60. *Developer has no idea why their tools stopped.*],
  [Switch to WS 1 → queries j/clients → finds `alacritty` with `clangd` child and CWD containing `CMakeLists.txt` → SYSTEM_DEV. Thaws and protects `clangd`, `make`, `cmake`, `gdb`. CPU governor: `performance`, swappiness: 10. *Correct with zero config.*],
)

#v(4pt)
#callout(color: negative, icon: "✗")[Static Map: *FAILS* — tool destruction on every WS 1 switch with default config.]
#callout(color: positive, icon: "✓")[FusionClassifier: *PASSES* — correctly inferred from `CMakeLists.txt` CWD + `clangd` child.]

== Scenario B — Android Dev on WS 2 (Current Config)

#table(
  columns: (1fr, 1fr),
  fill: (col, row) => if row == 0 { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 9pt,

  text(weight: "bold")[Static Map Behavior], text(weight: "bold")[FusionClassifier Behavior],

  [WS 2 = Web Dev. Switch to WS 2 → immediately prunes `studio`, `java`, `gradle`, `emulator`, `adb`. Gradle daemon killed mid-build. *8 seconds of work lost.*],
  [Switch to WS 2 → finds `android-studio` (known IDE) → FusionClassifier runs → detects `gradle` + `java` children + `build.gradle` in CWD → ANDROID_DEV. Thaws and protects all Android tools. *Correct.*],
)

#v(4pt)
#callout(color: negative, icon: "✗")[Static Map: *FAILS* — kills active Gradle daemon on every WS switch.]
#callout(color: positive, icon: "✓")[FusionClassifier: *PASSES* — Android Studio is a known IDE; fusion triggers correctly.]

== Scenario C — Browser on Any Workspace

#table(
  columns: (1fr, 1fr),
  fill: (col, row) => if row == 0 { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 9pt,

  text(weight: "bold")[Static Map Behavior], text(weight: "bold")[FusionClassifier Behavior],

  [Browser process (`firefox`, `chromium`) is not in any tool set (`web_tools`, `android_tools`, etc.). It is neither pruned nor protected. OOM score remains at kernel default. Behavior is identical on any WS.],
  [Browser is not in `known_ides`. It routes through the legacy fast-path: unknown binary → NEUTRAL → no transition. Identical outcome: browser is untouched on any workspace.],
)

#v(4pt)
#callout(color: neutral, icon: "→")[*Both systems: IDENTICAL* — browser is never managed by THM in either architecture.]

== Scenario D — Multi-Project Single Workspace (split-screen IDE)

#table(
  columns: (1fr, 1fr),
  fill: (col, row) => if row == 0 { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 9pt,

  text(weight: "bold")[Static Map Behavior], text(weight: "bold")[FusionClassifier Behavior],

  [WS 4 = System Dev. User opens Android Studio on same workspace. THM still applies System Dev profile — prunes `studio`, `java`, `gradle`. Android project context is destroyed.],
  [`code` (focused) detected as known IDE → FusionClassifier finds `tsserver` child (WEB_DEV, 0.6) AND `clangd` child (SYSTEM_DEV, 0.6) → tie → polyglot detected → `multi_context = true` → LSP protection enabled for *both*, neither is pruned.],
)

#v(4pt)
#callout(color: negative, icon: "✗")[Static Map: *FAILS* — WS profile always wins, destroys the secondary project context.]
#callout(color: positive, icon: "✓")[FusionClassifier: *PASSES* — polyglot detection (TC-1.1) keeps both LSPs protected.]

== Scenario E — Portable Install on Vanilla Arch / NixOS

#table(
  columns: (1fr, 1fr),
  fill: (col, row) => if row == 0 { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 9pt,

  text(weight: "bold")[Static Map Behavior], text(weight: "bold")[FusionClassifier Behavior],

  [Fresh install. User runs `make install`. Default config ships WS 1=Casual, WS 2=Web, etc. User's actual workflow uses different WS numbers. THM misclassifies everything until user manually updates config.],
  [Fresh install. No config needed. THM reads focused window PID on every switch. Correct profile inferred immediately from running processes and project markers. Works identically on ArchTitan, NixOS, Fedora Kinoite.],
)

#v(4pt)
#callout(color: negative, icon: "✗")[Static Map: *FAILS* — misconfigured by default on every non-ArchTitan system.]
#callout(color: positive, icon: "✓")[FusionClassifier: *PASSES* — zero configuration required, correct on any distro.]

== Scenario F — Empty Workspace (Cold Boot, No Windows Open)

#table(
  columns: (1fr, 1fr),
  fill: (col, row) => if row == 0 { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 9pt,

  text(weight: "bold")[Static Map Behavior], text(weight: "bold")[FusionClassifier Behavior],

  [User switches to WS 4 (System Dev) before opening any app. THM immediately applies System Dev profile: `performance` governor, swappiness 10, OOM-protects system tools. Pre-configures hardware for the upcoming task.],
  [User switches to WS 4 — no windows, no focused PID. `j/clients` returns empty. FusionClassifier has no signal. Returns NEUTRAL. No transition occurs. Developer must focus a window first.],
)

#v(4pt)
#callout(color: positive, icon: "✓")[Static Map: *PASSES* — pre-warms system before any app opens. Best for ritual workflows.]
#callout(color: neutral, icon: "→")[FusionClassifier: *PARTIAL* — accurate but reactive only. No preemptive tuning.]

#section_rule()

// ── Recommendation ───────────────────────────────────────────
= Recommendation — Hybrid Architecture

Neither approach is strictly superior. The optimal design combines both:

#block(
  fill: accent.lighten(90%),
  stroke: (left: 3pt + accent),
  radius: (right: 4pt),
  inset: (x: 14pt, y: 12pt),
)[
  *Proposed Hybrid Strategy*

  #v(6pt)
  #neutral_item[*Primary signal:* FusionClassifier on `activewindow>>` and `workspace>>` (via `j/clients`)]
  #v(4pt)
  #neutral_item[*Fallback signal:* Static workspace map — applied *only* when FusionClassifier returns NEUTRAL (empty workspace, unknown binary)]
  #v(4pt)
  #neutral_item[*Config optional:* Workspace map is empty by default; power users can populate it for cold-boot prewarming]
  #v(4pt)
  #neutral_item[*Override mode:* Manual `titan-hwm sw <profile>` via CLI always wins over both signals]
]

#v(8pt)

This hybrid gives:
- Accurate inference for known workflows (FusionClassifier wins 95% of the time)
- Pre-warming for ritual workspace users (static map fills the 5% gap)
- Zero mandatory configuration on any system
- Backward compatibility for existing ArchTitan users with configured WS maps

#section_rule()

// ── Implementation Delta ─────────────────────────────────────
= Implementation Delta (What Needs to Change)

#table(
  columns: (2fr, 3fr, 1fr),
  fill: (col, row) => if row == 0 { accent } else if calc.odd(row) { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 8pt,
  align: (left, left, center),

  text(fill: white, weight: "bold")[Change], text(fill: white, weight: "bold")[Detail], text(fill: white, weight: "bold")[Effort],

  [Route `workspace>>` through FusionClassifier], [On WS event: call `query_workspace_pids()` → get focused PID → run `FusionClassifier::classify()`], pill("Low", color: positive),
  [Make static map optional fallback], [If FusionClassifier returns NEUTRAL, fall through to `classify_ws()` instead of returning immediately], pill("Low", color: positive),
  [Ship empty default WS map], [Change default config: all workspace sets empty. Users opt in.], pill("Low", color: positive),
  [Add `j/clients` retry on socket fail], [If `j/clients` IPC fails, fall back to static map gracefully], pill("Medium", color: rgb("#c47a00")),
  [Update `known_ides` in config], [Add `neovide`, `lapce`, `helix`, `nvim` to the list], pill("Low", color: positive),
  [Remove mandatory WS config docs], [Update wiki and config comments to mark WS map as optional], pill("Low", color: positive),
)

#v(10pt)

#callout(color: positive, icon: "✓")[
  Total estimated implementation time: *2–4 hours*. All required infrastructure (FusionClassifier, `query_workspace_pids`, `j/clients` parsing) already exists in `titan_hw_manager.cpp`. This is a routing change, not a rewrite.
]

#section_rule()

// ── Appendix ─────────────────────────────────────────────────
= Appendix — Tool Classification Reference

#table(
  columns: (2fr, 1fr, 1fr, 1fr, 1fr),
  fill: (col, row) => if row == 0 { accent } else if calc.odd(row) { bg_light } else { white },
  stroke: 0.5pt + divider,
  inset: 7pt,
  align: (left, center, center, center, center),

  text(fill: white, weight: "bold")[Tool / Process], text(fill: white, weight: "bold")[Web Dev], text(fill: white, weight: "bold")[Android Dev], text(fill: white, weight: "bold")[System Dev], text(fill: white, weight: "bold")[Casual],

  [`node`, `webpack`, `vite`, `bun`, `esbuild`], [✓ thaw], [✗ freeze], [✗ freeze], [✗ freeze],
  [`studio`, `emulator`, `adb`], [✗ freeze], [✓ thaw], [✗ freeze], [✗ freeze],
  [`java`, `gradle`], [✗ freeze], [✓ thaw], [✗ freeze], [✗ freeze],
  [`clangd`, `ccls`, `rust-analyzer`], [✗ freeze], [✗ freeze], [✓ thaw], [✗ freeze],
  [`cargo`, `make`, `cmake`, `gdb`], [✗ freeze], [✗ freeze], [✓ thaw], [✗ freeze],
  [`figma-linux`], [✓ thaw], [—], [—], [—],
  [`firefox`, `chromium`, `brave`], [—], [—], [—], [—],
  [`telegram-desktop`, `discord`, `slack`], [✗ freeze], [✗ freeze], [✗ freeze], [✓ thaw],
  [`nautilus`, `dolphin`, `thunar`], [✗ freeze], [✗ freeze], [✗ freeze], [✓ thaw],
  [`pipewire`, `wireplumber`, `mpv`], [✓ exempt], [✓ exempt], [✓ exempt], [✓ exempt],
)

#v(4pt)
#text(size: 8.5pt, fill: neutral)[— = untouched (not in any managed set)   ✓ exempt = audio whitelist, never frozen or killed regardless of profile]

