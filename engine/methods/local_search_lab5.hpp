#pragma once

#include "core/method.hpp"

#include <vector>

// Laboratory-5 local search: steepest search driven by a list of improving
// moves (LM). The neighbourhood is the Laboratory-3 winner (inter-route node
// exchange plus intra-route 2-opt edge exchange), but move deltas are kept in
// memory between iterations instead of rescanning the whole neighbourhood.
//
// Two methods are registered:
//   * ls_lm_steepest_random  - LM over the full neighbourhood
//   * ls_lm_cand_random      - LM over the Laboratory-4 candidate moves
namespace lab5 {

void registerMethods(std::vector<Method>& methods);

}  // namespace lab5
