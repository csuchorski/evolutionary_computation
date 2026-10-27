#include "lab1/lab1.hpp"

#include <algorithm>
#include <limits>
#include <numeric>

// ---------------------------------------------------------------------------
//  Method 1: random solution
// ---------------------------------------------------------------------------
static std::vector<int> randomTour(const Instance& inst, std::mt19937& rng) {
    std::vector<int> idx(inst.n);
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng);   // uniform subset ...
    idx.resize(inst.k);
    std::shuffle(idx.begin(), idx.end(), rng);   // ... in random order
    return idx;
}

// ---------------------------------------------------------------------------
//  Method 2: nearest neighbour, append only at the end of the open path
// ---------------------------------------------------------------------------
static std::vector<int> nnAppend(const Instance& inst, int start) {
    const int n = inst.n, k = inst.k;
    std::vector<char> used(n, 0);
    std::vector<int> tour;
    tour.reserve(k);
    tour.push_back(start);
    used[start] = 1;
    int last = start;

    for (int step = 1; step < k; ++step) {
        long long best = std::numeric_limits<long long>::max();
        int bestJ = -1;
        for (int j = 0; j < n; ++j) {
            if (used[j]) continue;
            const long long delta = inst.cost[j] + inst.d[last][j];
            if (delta < best) { best = delta; bestJ = j; }
        }
        tour.push_back(bestJ);
        used[bestJ] = 1;
        last = bestJ;
    }
    return tour;
}

// ---------------------------------------------------------------------------
//  Method 3: nearest neighbour, best of every position of the open path
//            (front, interior, end)
// ---------------------------------------------------------------------------
static std::vector<int> nnAnyPosition(const Instance& inst, int start) {
    const int n = inst.n, k = inst.k;
    std::vector<char> used(n, 0);
    std::vector<int> tour;
    tour.reserve(k);
    tour.push_back(start);
    used[start] = 1;

    for (int step = 1; step < k; ++step) {
        const int m = static_cast<int>(tour.size());
        long long bestDelta = std::numeric_limits<long long>::max();
        int bestJ = -1, bestPos = -1;

        for (int j = 0; j < n; ++j) {
            if (used[j]) continue;
            const long long cj = inst.cost[j];
            const auto& dj = inst.d[j];

            // insert before the first node: new arc j -> tour[0]
            long long delta = cj + dj[tour[0]];
            if (delta < bestDelta) { bestDelta = delta; bestJ = j; bestPos = 0; }

            // insert between consecutive nodes a -> b: replace a->b by a->j, j->b
            for (int p = 1; p < m; ++p) {
                const int a = tour[p - 1];
                const int b = tour[p];
                delta = cj + inst.d[a][j] + dj[b] - inst.d[a][b];
                if (delta < bestDelta) { bestDelta = delta; bestJ = j; bestPos = p; }
            }

            // append at the end: new arc tour.last -> j
            delta = cj + inst.d[tour[m - 1]][j];
            if (delta < bestDelta) { bestDelta = delta; bestJ = j; bestPos = m; }
        }

        tour.insert(tour.begin() + bestPos, bestJ);
        used[bestJ] = 1;
    }
    return tour;
}

// ---------------------------------------------------------------------------
//  Method 4: greedy cycle
//  The tour is a closed directed cycle at every step. Inserting j into the
//  cycle edge a -> b removes that arc and adds BOTH a -> j and j -> b:
//      delta = cost[j] + d[a][j] + d[j][b] - d[a][b]
//  The wrap edge (last -> first) is an ordinary insertion position, so
//  "appending" costs d[last][j] + d[j][first] - d[last][first].
// ---------------------------------------------------------------------------
static std::vector<int> greedyCycle(const Instance& inst, int start) {
    const int n = inst.n, k = inst.k;
    std::vector<char> used(n, 0);
    used[start] = 1;

    // two-node cycle s -> j -> s (two distinct directed arcs)
    long long best = std::numeric_limits<long long>::max();
    int bestJ = -1;
    for (int j = 0; j < n; ++j) {
        if (j == start) continue;
        const long long v = inst.cost[j] + inst.d[start][j] + inst.d[j][start];
        if (v < best) { best = v; bestJ = j; }
    }
    std::vector<int> tour;
    tour.reserve(k);
    tour.push_back(start);
    tour.push_back(bestJ);
    used[bestJ] = 1;

    for (int step = 2; step < k; ++step) {
        const int m = static_cast<int>(tour.size());
        long long bestDelta = std::numeric_limits<long long>::max();
        int bestJ2 = -1, bestPos = -1;

        // Node-first scan: equal deltas keep the smallest node index, then the
        // earliest cycle edge.
        for (int j = 0; j < n; ++j) {
            if (used[j]) continue;
            const auto& dj = inst.d[j];
            for (int p = 0; p < m; ++p) {
                const int a = tour[p];
                const int b = tour[(p + 1) % m];   // wrap edge when p == m-1
                const long long delta = inst.cost[j] + inst.d[a][j] + dj[b] - inst.d[a][b];
                if (delta < bestDelta) { bestDelta = delta; bestJ2 = j; bestPos = p; }
            }
        }
        tour.insert(tour.begin() + bestPos + 1, bestJ2);
        used[bestJ2] = 1;
    }
    return tour;
}

void registerLab1(std::vector<Method>& methods) {
    methods.push_back({"random", "Random", 1, false,
                       [](BuildContext& c) { return randomTour(c.inst, c.rng); }});
    methods.push_back({"nn_end", "Nearest neighbour (append at end)", 1, true,
                       [](BuildContext& c) { return nnAppend(c.inst, c.start); }});
    methods.push_back({"nn_all", "Nearest neighbour (any position)", 1, true,
                       [](BuildContext& c) { return nnAnyPosition(c.inst, c.start); }});
    methods.push_back({"greedy_cycle", "Greedy cycle", 1, true,
                       [](BuildContext& c) { return greedyCycle(c.inst, c.start); }});
}
