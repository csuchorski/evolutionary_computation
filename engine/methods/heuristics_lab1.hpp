#pragma once

#include "core/method.hpp"

#include <random>
#include <vector>

// Laboratory-1 heuristic methods:
//   random, nn_end, nn_all, greedy_cycle
namespace lab1 {

std::vector<int> randomTour(const Instance& inst, std::mt19937& rng);
std::vector<int> greedyCycle(const Instance& inst, int start);
void registerMethods(std::vector<Method>& methods);

}  // namespace lab1
