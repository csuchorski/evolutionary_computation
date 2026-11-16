#pragma once

#include "core/instance.hpp"

#include <functional>
#include <random>
#include <string>
#include <vector>

// Per-run diagnostics. Non-local-search methods leave these at zero; local-
// search methods fill them in so the report can show the search effort.
struct BuildStats {
    long long initialValue = 0;   // objective of the starting solution
    long long moves = 0;          // accepted improving moves
    long long evaluations = 0;    // candidate deltas computed
};

// Everything a method needs. `start` is the starting node and is ignored by
// methods with `usesStart == false` (e.g. random starts).
struct BuildContext {
    const Instance& inst;
    int start = -1;
    std::mt19937& rng;
    double wr = 0.5;   // weighted-sum regret weight
    double wb = 0.5;   // weighted-sum best-change weight
    int candidates = 10;  // number of nearest vertices kept per candidate list
    BuildStats stats;
};

// One registered method. `lab` groups methods by assignment so the driver can
// enable them with --lab.
struct Method {
    std::string key;     // file-friendly identifier
    std::string label;   // human-readable name
    int lab = 1;         // assignment the method belongs to (1, 2, 3 or 4)
    bool usesStart = true;
    std::function<std::vector<int>(BuildContext&)> build;
    // Optional: key of another method whose RNG stream should be shared. Used by
    // Lab 4 so the candidate search starts from exactly the same random tours as
    // its baseline, making the comparison paired. Empty means "use own key".
    std::string rngGroup{};
};
