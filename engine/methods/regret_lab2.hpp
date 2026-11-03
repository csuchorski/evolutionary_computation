#pragma once

#include "core/method.hpp"

#include <vector>

// Laboratory-2 regret methods, each on the open-path and closed-cycle base:
//   regret_path, regret_cycle, weighted_path, weighted_cycle
namespace lab2 {

void registerMethods(std::vector<Method>& methods);

}  // namespace lab2
