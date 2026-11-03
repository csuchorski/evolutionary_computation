#pragma once

#include "core/instance.hpp"

#include <vector>

// ---------------------------------------------------------------------------
//  Solution
// ---------------------------------------------------------------------------
struct Solution {
    std::vector<int> tour;   // directed cyclic order, size k
    long long value = 0;     // total objective
    long long dist = 0;      // sum of the k arc lengths (closing arc included)
    long long cost = 0;      // sum of the k node costs
};

Solution evaluate(const Instance& inst, std::vector<int> tour);
void validate(const Instance& inst, const Solution& s);
