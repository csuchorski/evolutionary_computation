#pragma once

#include "core/method.hpp"

#include <vector>

// Laboratory-3 local search:
//   steepest/greedy x node-swap/edge-swap intra moves x random/greedy starts
namespace lab3 {

void registerMethods(std::vector<Method>& methods);

}  // namespace lab3
