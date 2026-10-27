#pragma once

#include <filesystem>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
//  Instance: integer distance matrix and node costs
// ---------------------------------------------------------------------------
struct Instance {
    std::string name;                 // file stem, e.g. "TSPA"
    std::string source;               // file name, e.g. "TSPA.csv"
    int n = 0;                        // number of nodes
    int k = 0;                        // number of selected nodes
    std::vector<long long> cost;
    std::vector<std::vector<int>> d;  // directed integer distance matrix
};

// ---------------------------------------------------------------------------
//  Solution
// ---------------------------------------------------------------------------
struct Solution {
    std::vector<int> tour;   // directed cyclic order, size k
    long long value = 0;     // total objective
    long long dist = 0;      // sum of the k arc lengths (closing arc included)
    long long cost = 0;      // sum of the k node costs
};

// Everything a construction routine needs. `start` is the starting node and is
// ignored by methods with `usesStart == false` (the random method).
struct BuildContext {
    const Instance& inst;
    int start = -1;
    std::mt19937& rng;
    double wr = 0.5;   // weighted-sum regret weight
    double wb = 0.5;   // weighted-sum best-change weight
};

// One registered construction method. `lab` groups methods by assignment so the
// driver can enable them with --lab.
struct Method {
    std::string key;     // file-friendly identifier
    std::string label;   // human-readable name
    int lab = 1;         // assignment the method belongs to (1 or 2)
    bool usesStart = true;
    std::function<std::vector<int>(BuildContext&)> build;
};

// ---------------------------------------------------------------------------
//  Shared helpers
// ---------------------------------------------------------------------------
Instance loadInstance(const fs::path& path);
Solution evaluate(const Instance& inst, std::vector<int> tour);
void validate(const Instance& inst, const Solution& s);

std::string fixed1(double v);
void writeFile(const fs::path& p, const std::string& content);
