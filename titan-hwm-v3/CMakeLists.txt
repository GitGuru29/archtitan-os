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
    core/protected_registry.cpp
    workload/ownership_graph.cpp
    classifier/execution_detector.cpp
    workload/workload_manager.cpp
    policy/policy_engine.cpp
)

# ── Phase 0+1 compile-check target ───────────────────────────────────────────
# Grows as phases are implemented
add_library(thm_core STATIC ${CORE_SRC})
target_include_directories(thm_core PUBLIC ${CMAKE_SOURCE_DIR})

# ── Main daemon (stub until Phase 12) ────────────────────────────────────────
# add_executable(titan-hwm-v3 daemon/daemon.cpp)
# target_link_libraries(titan-hwm-v3 PRIVATE thm_core pthread)
