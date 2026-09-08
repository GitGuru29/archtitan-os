// =============================================================================
// titan-hwm-v3/classifier/fusion_classifier.hpp
// Phase 3 — FusionClassifier (ported from v2, output repurposed)
//
// The 3-signal classification logic is UNCHANGED from v2.
// What changed: FusionResult is gone. Output is ClassificationResult (types.hpp).
// ClassificationResult is DESCRIPTIVE METADATA ONLY — it never selects processes
// to kill, freeze, or target. It populates Workload::type and Workload::confidence.
//
// Signal weights (same as v2):
//   S1 child walk   0.6+ per matching process
//   S2 window title 0.3  (0.5 if high-specificity ext present)
//   S3 cwd stat     0.2  tiebreaker
// =============================================================================
#pragma once

#include "../core/types.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <map>
#include <sys/types.h>

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// ClassifierConfig — tool name sets (sourced from Config in Phase 12)
// Mirror the existing /etc/titan-hwm/config fields
// ─────────────────────────────────────────────────────────────────────────────
struct ClassifierConfig {
    std::vector<std::string> lsp_binaries  = {
        "clangd", "ccls", "rust-analyzer",
        "tsserver", "typescript-language-server",
        "eslint_d", "pyright", "jdtls",
        "kotlin-language-server", "gopls"
    };
    std::vector<std::string> build_daemons = {
        "gradle", "gradlew", "java", "adb", "kotlin-compiler", "aapt2",
        "cargo", "cmake", "ninja", "make", "bazel", "buck2", "meson",
        "webpack", "vite", "esbuild", "tsc", "bun", "node"
    };
    std::vector<std::string> ai_inference  = {
        "ollama", "llama-server", "llamacpp", "llama.cpp",
        "aider", "sgpt", "openai", "litellm",
        "titan-ai", "titanai"
    };
    std::vector<std::string> latency_sensitive = {
        "titan-mirror", "titanmirror",
        "obs", "obs-studio",
        "ffmpeg", "gst-launch-1.0"
    };
};

// ─────────────────────────────────────────────────────────────────────────────
// FusionClassifier — 3-signal fusion (logic unchanged from v2)
// ─────────────────────────────────────────────────────────────────────────────
class FusionClassifier {
public:
    explicit FusionClassifier(ClassifierConfig cfg = {}) : cfg_(std::move(cfg)) {}

    // Main entry point — fuse all 3 signals and return ClassificationResult.
    // pid:   root PID of the workload being classified
    // title: active window title (from Hyprland activewindowv2 event)
    // graph: full /proc snapshot (pid → ProcessNode)
    ClassificationResult classify(pid_t                                       pid,
                                   const std::string&                          title,
                                   const std::unordered_map<pid_t, ProcessNode>& graph) const;

    // Individual signals (public for testing)
    ClassificationResult from_children(pid_t pid,
                                        const std::unordered_map<pid_t, ProcessNode>& graph) const;
    static std::optional<WorkloadType> from_title(const std::string& title);
    static std::optional<WorkloadType> from_cwd(pid_t pid);

private:
    ClassifierConfig cfg_;

    // Security: reject executables running from world-writable paths
    static bool is_valid_executable_path(pid_t pid);

    // Recursive child walk (depth 3) — same as v2
    static void walk_child_procs_recursive(
        pid_t pid,
        const std::unordered_map<pid_t, ProcessNode>& graph,
        std::vector<std::pair<pid_t, std::string>>& out,
        int depth = 3);

    // TC-1.3: boost S2 weight when title has unambiguous extension
    static bool title_has_specific_ext(const std::string& title);

    // TC-1.5: strip IDE variant suffixes (nightly, preview, etc.)
    static std::string strip_ide_suffix(const std::string& name);

    // Read argv[0] from /proc/<pid>/cmdline
    static std::string read_cmdline(pid_t pid);
};

} // namespace thm
