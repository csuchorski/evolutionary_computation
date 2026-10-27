#include "lab2/lab2.hpp"

#include <algorithm>
#include <limits>

// ---------------------------------------------------------------------------
//  Regret heuristics
//  For every unused node the two cheapest insertion positions are compared.
//  The open-path base reuses the extended nearest-neighbour insertion set
//  (front, interior, end); the closed-cycle base reuses the greedy-cycle
//  insertion set (every edge, wrap edge included).
// ---------------------------------------------------------------------------
struct RegretInfo {
    long long best = 0;     // cheapest insertion delta
    long long second = 0;   // second cheapest (equals best if only one position)
    int pos = -1;           // insertion index (path) or edge index (cycle)
};

static RegretInfo nodeRegret(const Instance& inst, const std::vector<int>& tour,
                             bool cycle, int j) {
    const int m = static_cast<int>(tour.size());
    long long b1 = std::numeric_limits<long long>::max();
    long long b2 = std::numeric_limits<long long>::max();
    int p1 = -1;
    auto consider = [&](long long delta, int pos) {
        if (delta < b1) { b2 = b1; b1 = delta; p1 = pos; }
        else if (delta < b2) { b2 = delta; }
    };

    if (cycle) {
        for (int p = 0; p < m; ++p) {
            const int a = tour[p];
            const int b = tour[(p + 1) % m];   // wrap edge when p == m-1
            consider(inst.cost[j] + inst.d[a][j] + inst.d[j][b] - inst.d[a][b], p);
        }
    } else {
        // insert before the first node
        consider(inst.cost[j] + inst.d[j][tour[0]], 0);
        // insert between consecutive nodes
        for (int p = 1; p < m; ++p) {
            const int a = tour[p - 1];
            const int b = tour[p];
            consider(inst.cost[j] + inst.d[a][j] + inst.d[j][b] - inst.d[a][b], p);
        }
        // append at the end
        consider(inst.cost[j] + inst.d[tour[m - 1]][j], m);
    }

    RegretInfo info;
    info.best = b1;
    info.second = (b2 == std::numeric_limits<long long>::max()) ? b1 : b2;
    info.pos = p1;
    return info;
}

// cycle: closed-cycle base (greedy cycle); otherwise open-path base (extended NN).
// weighted: score = wr * normalised regret + wb * (1 - normalised best change);
//           with weighted == false the node with the largest raw regret wins.
static std::vector<int> regretConstruct(const Instance& inst, int start, bool cycle,
                                        bool weighted, double wr, double wb) {
    const int n = inst.n, k = inst.k;
    std::vector<char> used(n, 0);
    std::vector<int> tour;
    tour.reserve(k);
    used[start] = 1;
    tour.push_back(start);

    if (cycle) {
        long long best = std::numeric_limits<long long>::max();
        int bestJ = -1;
        for (int j = 0; j < n; ++j) {
            if (j == start) continue;
            const long long v = inst.cost[j] + inst.d[start][j] + inst.d[j][start];
            if (v < best) { best = v; bestJ = j; }
        }
        tour.push_back(bestJ);
        used[bestJ] = 1;
    }

    std::vector<int> cand;
    std::vector<RegretInfo> info;
    cand.reserve(n);
    info.reserve(n);

    while (static_cast<int>(tour.size()) < k) {
        cand.clear();
        info.clear();
        long long minBest = std::numeric_limits<long long>::max();
        long long maxBest = std::numeric_limits<long long>::min();
        long long minReg = std::numeric_limits<long long>::max();
        long long maxReg = std::numeric_limits<long long>::min();

        for (int j = 0; j < n; ++j) {
            if (used[j]) continue;
            const RegretInfo ri = nodeRegret(inst, tour, cycle, j);
            const long long reg = ri.second - ri.best;
            cand.push_back(j);
            info.push_back(ri);
            minBest = std::min(minBest, ri.best);
            maxBest = std::max(maxBest, ri.best);
            minReg = std::min(minReg, reg);
            maxReg = std::max(maxReg, reg);
        }

        // Equal criteria fall back to the smaller best change, so the first
        // open-path step (where every regret is 0) degrades to the plain
        // nearest-neighbour choice instead of picking an arbitrary node.
        int chosen = 0;
        long long chosenBest = std::numeric_limits<long long>::max();
        if (!weighted) {
            long long bestReg = std::numeric_limits<long long>::min();
            for (std::size_t idx = 0; idx < cand.size(); ++idx) {
                const long long reg = info[idx].second - info[idx].best;
                if (reg > bestReg || (reg == bestReg && info[idx].best < chosenBest)) {
                    bestReg = reg;
                    chosenBest = info[idx].best;
                    chosen = static_cast<int>(idx);
                }
            }
        } else {
            double bestScore = -std::numeric_limits<double>::infinity();
            for (std::size_t idx = 0; idx < cand.size(); ++idx) {
                const long long reg = info[idx].second - info[idx].best;
                const double regN = (maxReg == minReg)
                    ? 0.0 : static_cast<double>(reg - minReg) / (maxReg - minReg);
                const double bestN = (maxBest == minBest)
                    ? 0.0 : static_cast<double>(info[idx].best - minBest) / (maxBest - minBest);
                const double score = wr * regN + wb * (1.0 - bestN);
                if (score > bestScore
                    || (score == bestScore && info[idx].best < chosenBest)) {
                    bestScore = score;
                    chosenBest = info[idx].best;
                    chosen = static_cast<int>(idx);
                }
            }
        }

        const int j = cand[chosen];
        const int pos = info[chosen].pos;
        tour.insert(tour.begin() + (cycle ? pos + 1 : pos), j);
        used[j] = 1;
    }
    return tour;
}

void registerLab2(std::vector<Method>& methods) {
    methods.push_back({"regret_path", "2-regret (open path)", 2, true,
                       [](BuildContext& c) { return regretConstruct(c.inst, c.start, false, false, 0.0, 0.0); }});
    methods.push_back({"regret_cycle", "2-regret (greedy cycle)", 2, true,
                       [](BuildContext& c) { return regretConstruct(c.inst, c.start, true, false, 0.0, 0.0); }});
    methods.push_back({"weighted_path", "Weighted sum (open path)", 2, true,
                       [](BuildContext& c) { return regretConstruct(c.inst, c.start, false, true, c.wr, c.wb); }});
    methods.push_back({"weighted_cycle", "Weighted sum (greedy cycle)", 2, true,
                       [](BuildContext& c) { return regretConstruct(c.inst, c.start, true, true, c.wr, c.wb); }});
}
 