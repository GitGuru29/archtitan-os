// =============================================================================
// titan-hwm-v3/classifier/fusion_classifier.cpp
// Phase 3 — FusionClassifier implementation (ported from v2)
//
// All classification logic is preserved verbatim from v2 (bugs fixed in v2
// still apply here: TC-1.1, TC-1.3, TC-1.4, TC-1.5, ISSUE-11).
// The ONLY change: output type is ClassificationResult, not FusionResult.
// =============================================================================
#include "fusion_classifier.hpp"

#include <fstream>
#include <iostream>
#include <filesystem>
#include <climits>
#include <unistd.h>
#include <algorithm>

namespace fs = std::filesystem;

namespace thm {

// ─────────────────────────────────────────────────────────────────────────────
// read_cmdline — argv[0] from /proc/<pid>/cmdline (NUL-delimited)
// ─────────────────────────────────────────────────────────────────────────────
std::string FusionClassifier::read_cmdline(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/cmdline", std::ios::binary);
    if (!f.is_open()) return {};
    std::string s;
    std::getline(f, s, '\0'); // argv[0] only
    // Basename only — strip path prefix
    auto slash = s.rfind('/');
    if (slash != std::string::npos) s = s.substr(slash + 1);
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// ISSUE-11 FIX: reject executables from world-writable paths
// Prevents unprivileged spoofing via /tmp/clangd etc.
// ─────────────────────────────────────────────────────────────────────────────
bool FusionClassifier::is_valid_executable_path(pid_t pid) {
    char exe_buf[PATH_MAX];
    std::string link = "/proc/" + std::to_string(pid) + "/exe";
    ssize_t len = readlink(link.c_str(), exe_buf, sizeof(exe_buf) - 1);
    if (len <= 0) return true; // process gone → don't penalise
    exe_buf[len] = '\0';
    std::string exe(exe_buf);
    if (exe.rfind("/tmp/",     0) == 0 || exe == "/tmp"     ||
        exe.rfind("/var/tmp/", 0) == 0 || exe == "/var/tmp" ||
        exe.rfind("/dev/shm/", 0) == 0 || exe == "/dev/shm") {
        std::cerr << "[FusionClassifier] Spoofed exe rejected PID " << pid
                  << ": " << exe << "\n";
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// walk_child_procs_recursive — depth-3 child walk (same as v2)
// ─────────────────────────────────────────────────────────────────────────────
void FusionClassifier::walk_child_procs_recursive(
        pid_t pid,
        const std::unordered_map<pid_t, ProcessNode>& graph,
        std::vector<std::pair<pid_t, std::string>>& out,
        int depth)
{
    if (depth <= 0) return;
    auto it = graph.find(pid);
    if (it == graph.end()) return;
    for (pid_t child : it->second.children) {
        std::string cmd = read_cmdline(child);
        if (!cmd.empty()) out.push_back({child, cmd});
        walk_child_procs_recursive(child, graph, out, depth - 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TC-1.3 FIX: high-specificity extension boost for S2 weight
// ─────────────────────────────────────────────────────────────────────────────
bool FusionClassifier::title_has_specific_ext(const std::string& title) {
    static const std::vector<std::string> specific = {
        ".ts", ".tsx", ".jsx", ".vue", ".svelte",
        ".rs", ".cpp", ".cc", ".cxx", ".c", ".h",
        "build.gradle", "AndroidManifest", "Cargo.toml", "CMakeLists"
    };
    for (const auto& s : specific)
        if (title.find(s) != std::string::npos) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// TC-1.5 FIX: strip IDE variant suffixes so "code-nightly" still classifies
// ─────────────────────────────────────────────────────────────────────────────
std::string FusionClassifier::strip_ide_suffix(const std::string& name) {
    static const std::vector<std::string> sfxs =
        {"-nightly", "-preview", "-dev", "-bin", "-stable", "-git", "-insiders"};
    for (const auto& sfx : sfxs)
        if (name.size() > sfx.size() &&
            name.compare(name.size() - sfx.size(), sfx.size(), sfx) == 0)
            return name.substr(0, name.size() - sfx.size());
    return name;
}

// ─────────────────────────────────────────────────────────────────────────────
// Signal 1 — child walk: score workload types from child processes
// ─────────────────────────────────────────────────────────────────────────────
ClassificationResult FusionClassifier::from_children(
        pid_t pid,
        const std::unordered_map<pid_t, ProcessNode>& graph) const
{
    ClassificationResult r;

    std::unordered_set<std::string> lsp_set(cfg_.lsp_binaries.begin(),  cfg_.lsp_binaries.end());
    std::unordered_set<std::string> bld_set(cfg_.build_daemons.begin(), cfg_.build_daemons.end());
    std::unordered_set<std::string> ai_set (cfg_.ai_inference.begin(),  cfg_.ai_inference.end());
    std::unordered_set<std::string> lat_set(cfg_.latency_sensitive.begin(), cfg_.latency_sensitive.end());
    // v3.1: service / VM sets
    std::unordered_set<std::string> svc_set(cfg_.container_runtimes.begin(), cfg_.container_runtimes.end());
    std::unordered_set<std::string> vm_set (cfg_.vm_processes.begin(), cfg_.vm_processes.end());

    std::unordered_map<int, float> scores; // keyed by WorkloadType cast to int

    // §5.2.1 — recursive walk depth 3 to catch grandchild LSP servers
    std::vector<std::pair<pid_t, std::string>> children;
    walk_child_procs_recursive(pid, graph, children, 3);

    for (const auto& [cpid, cmd] : children) {
        if (cmd.empty()) continue;
        // ISSUE-11: verify exe path
        if (!is_valid_executable_path(cpid)) continue;

        // HC-08: VM host process
        if (vm_set.count(cmd)) {
            scores[static_cast<int>(WorkloadType::VM)] += 0.9f;
            continue;
        }

        // HC-07: Container/service runtime child
        if (svc_set.count(cmd)) {
            scores[static_cast<int>(WorkloadType::SERVICE)] += 0.9f;
            continue;
        }

        // Latency-sensitive modifier (TitanMirror, OBS, ffmpeg)
        if (lat_set.count(cmd)) { r.latency_sensitive = true; continue; }

        // AI modifier — does NOT override workload type classification
        if (ai_set.count(cmd))  { r.ai_modifier = true; continue; }

        // LSP detection (weight 0.6)
        if (lsp_set.count(cmd)) {
            r.has_lsp = true;
            if (cmd == "clangd" || cmd == "ccls" || cmd == "rust-analyzer")
                scores[static_cast<int>(WorkloadType::SYSTEM_DEV)]  += 0.6f;
            else if (cmd == "tsserver" || cmd == "typescript-language-server" ||
                     cmd == "eslint_d")
                scores[static_cast<int>(WorkloadType::WEB_DEV)]     += 0.6f;
            else if (cmd == "jdtls" || cmd == "kotlin-language-server")
                scores[static_cast<int>(WorkloadType::ANDROID_DEV)] += 0.6f;
            continue;
        }

        // Build daemon detection (weight 0.6)
        if (bld_set.count(cmd)) {
            r.is_building = true;
            if (cmd == "gradle" || cmd == "gradlew" || cmd == "java" || cmd == "adb")
                scores[static_cast<int>(WorkloadType::ANDROID_DEV)] += 0.6f;
            else if (cmd == "cargo" || cmd == "cmake" || cmd == "ninja" || cmd == "make")
                scores[static_cast<int>(WorkloadType::SYSTEM_DEV)]  += 0.6f;
            else if (cmd == "webpack" || cmd == "vite" || cmd == "esbuild" ||
                     cmd == "tsc"     || cmd == "bun"  || cmd == "node")
                scores[static_cast<int>(WorkloadType::WEB_DEV)]     += 0.6f;
            continue;
        }

        // Supplementary lower-confidence signals (weight 0.3)
        if (cmd == "kotlin-compiler" || cmd == "aapt2") {
            r.is_building = true;
            scores[static_cast<int>(WorkloadType::ANDROID_DEV)] += 0.6f;
        } else if (cmd == "esbuild" || cmd == "bun") {
            scores[static_cast<int>(WorkloadType::WEB_DEV)]     += 0.3f;
        } else if (cmd == "gdb" || cmd == "make" || cmd == "ninja") {
            scores[static_cast<int>(WorkloadType::SYSTEM_DEV)]  += 0.3f;
        }
    }

    if (!scores.empty()) {
        auto best = std::max_element(scores.begin(), scores.end(),
            [](const auto& a, const auto& b){ return a.second < b.second; });
        r.type       = static_cast<WorkloadType>(best->first);
        r.confidence = best->second;
        r.score_map  = scores;
    }
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Signal 2 — window title: file extensions and project markers
// ─────────────────────────────────────────────────────────────────────────────
std::optional<WorkloadType> FusionClassifier::from_title(const std::string& title) {
    auto has = [&](const std::string& s){ return title.find(s) != std::string::npos; };

    // has_ext: matches suffix OR extension preceded by separator
    auto has_ext = [&](const std::string& ext) -> bool {
        if (title.size() >= ext.size() &&
            title.compare(title.size() - ext.size(), ext.size(), ext) == 0)
            return true;
        for (char sep : {' ', '/', '\\', ':', '|'}) {
            if (title.find(sep + ext) != std::string::npos) return true;
        }
        return false;
    };

    // Android — highest specificity, checked first
    if (has("build.gradle") || has("AndroidManifest") || has(".apk") || has("gradlew"))
        return WorkloadType::ANDROID_DEV;

    // Web
    if (has_ext(".ts") || has_ext(".tsx") || has_ext(".jsx") || has_ext(".vue") ||
        has("package.json") || has_ext(".html") || has_ext(".svelte"))
        return WorkloadType::WEB_DEV;

    // System/kernel dev
    if (has_ext(".cpp") || has_ext(".rs") || has_ext(".c") || has_ext(".h") ||
        has_ext(".cc")  || has_ext(".cxx") || has("CMakeLists") || has("Cargo.toml"))
        return WorkloadType::SYSTEM_DEV;

    return std::nullopt;
}

// ─────────────────────────────────────────────────────────────────────────────
// Signal 3 — CWD project marker stat
// TC-1.4: reject world-writable CWDs (container/overlay protection)
// ─────────────────────────────────────────────────────────────────────────────
std::optional<WorkloadType> FusionClassifier::from_cwd(pid_t pid) {
    std::string cwd;
    try {
        cwd = fs::read_symlink("/proc/" + std::to_string(pid) + "/cwd").string();
    } catch (...) { return std::nullopt; }
    if (cwd.empty()) return std::nullopt;

    // ISSUE-11 / TC-1.4: reject world-writable paths
    if (cwd.rfind("/tmp/",     0) == 0 || cwd == "/tmp"     ||
        cwd.rfind("/var/tmp/", 0) == 0 || cwd == "/var/tmp" ||
        cwd.rfind("/dev/shm/", 0) == 0 || cwd == "/dev/shm") {
        std::cout << "[S3] Ignored CWD in world-writable path: " << cwd << "\n";
        return std::nullopt;
    }

    fs::path p(cwd);
    std::error_code ec;
    if (!fs::exists(p, ec) || ec) {
        std::cout << "[S3] cwd " << cwd
                  << " unreachable from host ns (container/overlay?) — S3 skipped\n";
        return std::nullopt;
    }

    if (fs::exists(p / "build.gradle") || fs::exists(p / "app" / "src"))
        return WorkloadType::ANDROID_DEV;
    if (fs::exists(p / "package.json") && fs::exists(p / "node_modules"))
        return WorkloadType::WEB_DEV;
    if (fs::exists(p / "CMakeLists.txt") || fs::exists(p / "Cargo.toml"))
        return WorkloadType::SYSTEM_DEV;

    return std::nullopt;
}

// ─────────────────────────────────────────────────────────────────────────────
// classify — fuse all 3 signals with weighted voting
// TC-1.1: seed from full S1 score_map (not just winner) to preserve all votes
// TC-1.3: boost S2 weight to 0.5 when high-specificity ext is present
// HC-05/06: detect browser root from root cmdline — set is_browser_root + latency_sensitive
// ─────────────────────────────────────────────────────────────────────────────
ClassificationResult FusionClassifier::classify(
        pid_t pid,
        const std::string& title,
        const std::unordered_map<pid_t, ProcessNode>& graph) const
{
    // Signal 1
    ClassificationResult r = from_children(pid, graph);

    // HC-05/06: check if the root PID itself is a browser process
    // If so, mark it as browser_root and latency_sensitive (video/WebRTC calls)
    std::string root_cmd = read_cmdline(pid);
    std::unordered_set<std::string> br_set(cfg_.browser_roots.begin(), cfg_.browser_roots.end());
    if (br_set.count(root_cmd)) {
        r.is_browser_root    = true;
        r.latency_sensitive  = true; // browser may have active WebRTC call
    }

    // TC-1.1: seed unified vote from full S1 score_map
    std::unordered_map<int, float> vote;
    for (auto& [t, s] : r.score_map)
        if (static_cast<WorkloadType>(t) != WorkloadType::NEUTRAL)
            vote[t] += s;

    // Signal 2 — window title (TC-1.3: boosted weight for specific exts)
    auto t2 = from_title(title);
    float s2_weight = title_has_specific_ext(title) ? 0.5f : 0.3f;
    if (t2 && *t2 != WorkloadType::NEUTRAL)
        vote[static_cast<int>(*t2)] += s2_weight;

    // Signal 3 — CWD tiebreaker (weight 0.2)
    auto t3 = from_cwd(pid);
    if (t3 && *t3 != WorkloadType::NEUTRAL)
        vote[static_cast<int>(*t3)] += 0.2f;

    // Pick winner
    if (!vote.empty()) {
        auto best = std::max_element(vote.begin(), vote.end(),
            [](const auto& a, const auto& b){ return a.second < b.second; });
        r.type       = static_cast<WorkloadType>(best->first);
        r.confidence = best->second;
    }

    return r;
}

} // namespace thm
