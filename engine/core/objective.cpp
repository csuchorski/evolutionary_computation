#include "core/objective.hpp"

#include <stdexcept>

// ---------------------------------------------------------------------------
//  Objective function: closed directed cycle
// ---------------------------------------------------------------------------
Solution evaluate(const Instance& inst, std::vector<int> tour) {
    Solution s;
    s.tour = std::move(tour);
    const int m = static_cast<int>(s.tour.size());
    for (int i = 0; i < m; ++i) s.cost += inst.cost[s.tour[i]];
    for (int i = 0; i < m; ++i) {
        const int a = s.tour[i];
        const int b = s.tour[(i + 1) % m];
        s.dist += inst.d[a][b];
    }
    s.value = s.dist + s.cost;
    return s;
}

void validate(const Instance& inst, const Solution& s) {
    if (static_cast<int>(s.tour.size()) != inst.k)
        throw std::runtime_error("solution has wrong number of nodes");
    std::vector<char> seen(inst.n, 0);
    for (int v : s.tour) {
        if (seen[v]) throw std::runtime_error("duplicate node in tour");
        seen[v] = 1;
    }
    const Solution check = evaluate(inst, s.tour);
    if (check.value != s.value || check.dist != s.dist || check.cost != s.cost)
        throw std::runtime_error("objective recomputation mismatch");
}
