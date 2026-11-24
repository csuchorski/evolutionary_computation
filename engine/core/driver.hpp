#pragma once

#include "core/method.hpp"
#include "core/objective.hpp"

#include <random>
#include <string>
#include <vector>

struct MethodResult {
    std::string key;          // file-friendly identifier
    std::string label;        // human-readable name
    std::vector<Solution> sols;
    std::vector<int> startNode;
    std::vector<long long> initialValue;
    std::vector<long long> moves;
    std::vector<long long> evaluations;
    std::vector<long long> lmChecks;
    std::vector<long long> lmSkipped;
    std::vector<long long> lmRemoved;
    std::vector<long long> regenerations;
    std::vector<long long> maxLmSize;
    long long minV = 0, maxV = 0;
    double avgV = 0.0;
    int bestRun = 0;
    double ms = 0.0;

    const Solution& best() const { return sols[bestRun]; }
};

// A stable per-method RNG keeps results independent of which labs are selected
// with --lab, while remaining reproducible from the base seed.
std::mt19937 makeMethodRng(unsigned baseSeed, const std::string& key);

MethodResult runMethod(const Instance& inst, const Method& method, int runs,
                       unsigned seed, double wr, double wb, int candidates);
