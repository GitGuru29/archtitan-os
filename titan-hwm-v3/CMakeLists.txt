cmake_minimum_required(VERSION 3.18)
project(titan-hwm-v3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ────────────────────────────────────────────────────────────
add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
endif()

# ── Include directories ───────────────────────────────────────────────────────
include_directories(${CMAKE_SOURCE_DIR})

# ── Source files ─────────────────────────────────────────────────────────────
set(CORE_SRC
    # Phase 0+1: Protected Domain + Core Types
    core/protected_registry.cpp

    # Phase 2: Process Ownership Graph
    workload/ownership_graph.cpp

    # Phase 3: FusionClassifier (ported from v2)
    classifier/fusion_classifier.cpp

    # Phase 4: Execution Detector
    classifier/execution_detector.cpp

    # Phase 5: WorkloadState Machine
    workload/workload_manager.cpp

    # Phase 6: Policy Engine
    policy/policy_engine.cpp

    # Phase 7: cgroup Controller
    enforcement/cgroup_controller.cpp

    # Phase 8: Workspace Monitor + Governor
    ipc/workspace_monitor.cpp

    # Phase 9: Enforcement Plane
    enforcement/enforcement_plane.cpp

    # Phase 10: Reclaim Engine
    policy/reclaim_engine.cpp

    # Phase 11: Pressure Level Engine
    resources/pressure_level.cpp
)

# ── Static library (all phases) ──────────────────────────────────────────────
add_library(thm_core STATIC ${CORE_SRC})
target_include_directories(thm_core PUBLIC ${CMAKE_SOURCE_DIR})

# ── Main daemon executable (Phase 12) ────────────────────────────────────────
add_executable(titan-hwm-v3 daemon/daemon.cpp)
target_link_libraries(titan-hwm-v3 PRIVATE thm_core pthread)

# ── Install targets ──────────────────────────────────────────────────────────
install(TARGETS titan-hwm-v3 RUNTIME DESTINATION bin)
install(FILES archtitan.slice
        DESTINATION lib/systemd/system
        COMPONENT systemd)
