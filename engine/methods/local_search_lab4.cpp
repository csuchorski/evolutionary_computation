#include "methods/local_search_lab4.hpp"

#include "core/objective.hpp"
#include "methods/heuristics_lab1.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace lab4 {
namespace {

// ---------------------------------------------------------------------------
//  Candidate edges
//
//  For every vertex we keep its `count` "nearest" vertices under two different
//  keys, because the two move families change the objective differently:
//
//    * inter-route moves exchange a selected node for an unselected one, so the
//      selected set changes and the neighbour's cost matters:
//          key(i, j) = d(i, j) + cost[j]
//    * intra-route moves keep the selected set, so node costs cancel and only
//      the distances matter:
//          key(i, j) = d(i, j)
//
//  The lists are therefore built twice, once per family. Both are directed:
//  the candidate edge (i, j) exists when j is in i's list.
// ---------------------------------------------------------------------------
std::vector<std::vector<int>> buildCandidateList(const Instance& inst, int count,
                                                 bool withCost) {
    const int n = inst.n;
    const int take = std::min(count, n - 1);

    std::vector<std::vector<int>> list(n);
    std::vector<std::pair<long long, int>> ranked;
    ranked.reserve(n);

    for (int i = 0; i < n; ++i) {
        ranked.clear();
        for (int j = 0; j < n; ++j) {
            if (j == i) continue;
            const long long key = inst.d[i][j]
                                + (withCost ? inst.cost[j] : 0);
            ranked.emplace_back(key, j);   // ties broken by the smaller index
        }
        std::partial_sort(ranked.begin(), ranked.begin() + take, ranked.end());

        list[i].reserve(take);
        for (int t = 0; t < take; ++t) list[i].push_back(ranked[t].second);
    }
    return list;
}

// The candidate lists depend only on the instance and the list size, but they
// are rebuilt for every run so that the reported running time includes their
// construction. No static cache is used: caching would amortise the build over
// all runs and make the candidate method look artificially fast.
struct CandidateSet {
    std::string name;
    int count = -1;
    std::vector<std::vector<int>> inter;   // d + cost
    std::vector<std::vector<int>> intra;   // d only
};

CandidateSet candidateSet(const Instance& inst, int count) {
    CandidateSet set;
    set.name = inst.name;
    set.count = count;
    set.inter = buildCandidateList(inst, count, true);
    set.intra = buildCandidateList(inst, count, false);
    return set;
}

// ---------------------------------------------------------------------------
//  O(1) objective deltas (same formulas as the Laboratory-3 local search)
// ---------------------------------------------------------------------------

// Exchange the selected node tour[pos] = u for the unselected node v.
long long interDelta(const Instance& inst, const std::vector<int>& tour,
                     int pos, int v) {
    const int k = static_cast<int>(tour.size());
    const int u = tour[pos];
    const int prev = tour[(pos - 1 + k) % k];
    const int next = tour[(pos + 1) % k];
    return inst.cost[v] - inst.cost[u]
         + inst.d[prev][v] + inst.d[v][next]
         - inst.d[prev][u] - inst.d[u][next];
}

// Two-edge exchange (2-opt) of the edges starting at positions lo and hi. The
// two new edges are (tour[lo], tour[hi]) and (tour[lo+1], tour[hi+1]); the
// segment between them is reversed. For the symmetric instances this is the
// standard four-arc delta; the directed fallback keeps the formula exact.
long long edgeDelta(const Instance& inst, const std::vector<int>& tour,
                    int p, int q) {
    const int k = static_cast<int>(tour.size());
    const int lo = std::min(p, q);
    const int hi = std::max(p, q);
    const int a = tour[lo];
    const int b = tour[(lo + 1) % k];
    const int c = tour[hi];
    const int e = tour[(hi + 1) % k];

    long long delta = inst.d[a][c] + inst.d[b][e]
                    - inst.d[a][b] - inst.d[c][e];
    if (!inst.symmetric) {
        for (int t = lo + 1; t < hi; ++t) {
            delta += inst.d[tour[t + 1]][tour[t]]
                   - inst.d[tour[t]][tour[t + 1]];
        }
    }
    return delta;
}

bool contains(const std::vector<int>& list, int node) {
    return std::find(list.begin(), list.end(), node) != list.end();
}

// A 2-opt pair is degenerate when the two edges coincide or share a node.
bool validEdgePair(int p, int q, int k) {
    if (p == q) return false;
    if ((p + 1) % k == q) return false;
    if ((q + 1) % k == p) return false;
    return true;
}

enum MoveType : int {
    InterExchange = 0,   // remove tour[a], insert the unselected node b
    EdgeSwap = 1         // 2-opt between the edges starting at a and b
};

struct Move {
    int type;
    int a;
    int b;
};

void applyMove(std::vector<int>& tour, const Move& move) {
    if (move.type == InterExchange) {
        tour[move.a] = move.b;
    } else {
        const int lo = std::min(move.a, move.b);
        const int hi = std::max(move.a, move.b);
        std::reverse(tour.begin() + lo + 1, tour.begin() + hi + 1);
    }
}

// ---------------------------------------------------------------------------
//  Steepest local search over candidate moves only
//
//  The loops themselves walk candidate edges: for every selected node `a` we
//  visit `a`'s nearest vertices, so the whole scan is O(k * count) instead of
//  the full O(k*(n-k) + k^2) neighbourhood. Every move that is evaluated
//  introduces the candidate edge (a, j), as required.
// ---------------------------------------------------------------------------
std::vector<int> candidateSteepest(const Instance& inst,
                                   const std::vector<std::vector<int>>& inter,
                                   const std::vector<std::vector<int>>& intra,
                                   std::vector<int> tour, BuildStats& stats) {
    const int k = static_cast<int>(tour.size());
    const long long startValue = evaluate(inst, tour).value;
    long long value = startValue;
    stats.initialValue = startValue;

    std::vector<char> selected(inst.n, 0);
    std::vector<int> posOf(inst.n, -1);
    auto refresh = [&]() {
        std::fill(selected.begin(), selected.end(), 0);
        std::fill(posOf.begin(), posOf.end(), -1);
        for (int p = 0; p < k; ++p) {
            selected[tour[p]] = 1;
            posOf[tour[p]] = p;
        }
    };
    refresh();

    while (true) {
        long long bestDelta = 0;
        Move bestMove{InterExchange, -1, -1};
        bool found = false;

        auto consider = [&](long long delta, const Move& move) {
            ++stats.evaluations;
            if (delta < bestDelta) {
                bestDelta = delta;
                bestMove = move;
                found = true;
            }
        };

        for (int p = 0; p < k; ++p) {
            const int a = tour[p];

            // Intra-route: both endpoints selected. The two 2-opt moves that
            // introduce the candidate edge (a, j) remove the edges after a/j
            // or the edges before a/j. When the candidate relation is mutual
            // the pair is claimed only by the smaller node, so no move is
            // evaluated twice.
            for (int j : intra[a]) {
                if (!selected[j]) continue;
                // Keep only the first occurrence in iteration (position) order.
                if (contains(intra[j], a) && posOf[j] < p) continue;
                const int q = posOf[j];

                if (validEdgePair(p, q, k))
                    consider(edgeDelta(inst, tour, p, q), {EdgeSwap, p, q});

                const int p2 = (p - 1 + k) % k;
                const int q2 = (q - 1 + k) % k;
                if (validEdgePair(p2, q2, k))
                    consider(edgeDelta(inst, tour, p2, q2), {EdgeSwap, p2, q2});
            }

            // Inter-route: `a` stays in the tour and the candidate edge (a, j)
            // is introduced by removing one of a's two tour neighbours. The
            // move that removes prev(a) is also produced by the successor rule
            // from tour[p-2], so it is skipped then to avoid duplicates.
            for (int j : inter[a]) {
                if (selected[j]) continue;

                const int nextPos = (p + 1) % k;
                consider(interDelta(inst, tour, nextPos, j),
                         {InterExchange, nextPos, j});

                const int prevPos = (p - 1 + k) % k;
                if (k >= 3 && contains(inter[tour[(p - 2 + k) % k]], j))
                    continue;
                consider(interDelta(inst, tour, prevPos, j),
                         {InterExchange, prevPos, j});
            }
        }

        // Inter-route moves whose candidate edge is stored only in the
        // unselected endpoint's list. A reciprocal pair was already handled
        // from the selected anchor above.
        for (int u = 0; u < inst.n; ++u) {
            if (selected[u]) continue;
            for (int v : inter[u]) {
                if (!selected[v]) continue;
                if (contains(inter[v], u)) continue;

                const int q = posOf[v];
                const int nextPos = (q + 1) % k;
                consider(interDelta(inst, tour, nextPos, u),
                         {InterExchange, nextPos, u});

                const int prevPos = (q - 1 + k) % k;
                if (k >= 3 && contains(inter[tour[(q - 2 + k) % k]], u))
                    continue;
                consider(interDelta(inst, tour, prevPos, u),
                         {InterExchange, prevPos, u});
            }
        }

        if (!found) break;
        applyMove(tour, bestMove);
        value += bestDelta;
        ++stats.moves;
        refresh();
    }

    const Solution check = evaluate(inst, tour);
    if (check.value != value)
        throw std::runtime_error("candidate local search delta mismatch");
    return tour;
}

std::vector<int> buildCandidateSteepest(BuildContext& ctx) {
    const CandidateSet set = candidateSet(ctx.inst, ctx.candidates);
    std::vector<int> tour = lab1::randomTour(ctx.inst, ctx.rng);
    return candidateSteepest(ctx.inst, set.inter, set.intra, std::move(tour),
                             ctx.stats);
}

}  // namespace

void registerMethods(std::vector<Method>& methods) {
    methods.push_back({
        "ls_cand_steepest_random",
        "Steepest, candidate 2-opt, random start",
        4, false,
        [](BuildContext& ctx) { return buildCandidateSteepest(ctx); },
        "ls_steepest_edges_random"   // share the baseline's stream: same 200 starts
    });
}

}  // namespace lab4
