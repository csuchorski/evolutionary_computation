#pragma once

#include "core/method.hpp"

#include <vector>

// Laboratory-4 local search: steepest search restricted to candidate moves.
// The neighbourhood is the one that won Laboratory 3 (inter-route node exchange
// plus intra-route 2-opt edge exchange), but only moves that introduce at least
// one candidate edge are examined. Candidate edges are built once per instance,
// separately for inter- and intra-route moves.
namespace lab4 {

void registerMethods(std::vector<Method>& methods);

}  // namespace lab4
