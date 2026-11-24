#include "methods/local_search_lab3.hpp"

#include "core/objective.hpp"
#include "methods/heuristics_lab1.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <stdexcept>

namespace lab3 {
namespace {

enum class SearchMode { Steepest, Greedy };
enum class IntraMove { Nodes, Edges };
enum class StartMode { Random, Greedy };

enum MoveType : int {
    InterExchange = 0,   // one selected node with one unselected node
    NodeSwap = 1,        // exchange two positions inside the cycle
    EdgeSwap = 2         // two-edge exchange (2-opt)
};

struct Move {
    int type;
    int i;
    // InterExchange: j is an index in the unselected-node list.
    // NodeSwap / EdgeSwap: j is a tour position.
    int j;
};

// ---------------------------------------------------------------------------
//  O(1) objective deltas. No neighbour tour is constructed and the objective
//  is never recomputed from scratch while a move is being evaluated.
// ---------------------------------------------------------------------------

// Replace tour[i] = u by the currently unselected node v.
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

// Swap the nodes at positions i and j. Only the arcs incident to i and j can
// change; the set {i-1, i, j-1, j} is deduplicated so adjacent swaps are also
// handled exactly. Node costs are unchanged because the selected set is fixed.
long long nodeSwapDelta(const Instance& inst, const std::vector<int>& tour,
                        int i, int j) {
    const int k = static_cast<int>(tour.size());
    const int positions[4] = {(i - 1 + k) % k, i, (j - 1 + k) % k, j};
    long long delta = 0;

    for (int a = 0; a < 4; ++a) {
        bool duplicate = false;
        for (int b = 0; b < a; ++b) {
            if (positions[b] == positions[a]) { duplicate = true; break; }
        }
        if (duplicate) continue;

        const int p = positions[a];
        const int q = (p + 1) % k;

        int oldA = tour[p];
        int oldB = tour[q];
        int newA = tour[p];
        int newB = tour[q];

        if (p == i) newA = tour[j];
        else if (p == j) newA = tour[i];
        if (q == i) newB = tour[j];
        else if (q == j) newB = tour[i];

        delta += inst.d[newA][newB] - inst.d[oldA][oldB];
    }
    return delta;
}

// Two-edge exchange: remove edges (a,b) and (c,e), add (a,c) and (b,e), and
// reverse the segment b..c. For the symmetric Euclidean instances this is the
// standard four-arc 2-opt delta. The directed fallback is exact but only used
// if an asymmetric matrix is ever loaded.
long long edgeSwapDelta(const Instance& inst, const std::vector<int>& tour,
                        int i, int j) {
    const int k = static_cast<int>(tour.size());
    const int a = tour[i];
    const int b = tour[(i + 1) % k];
    const int c = tour[j];
    const int e = tour[(j + 1) % k];

    long long delta = inst.d[a][c] + inst.d[b][e]
                    - inst.d[a][b] - inst.d[c][e];
    if (!inst.symmetric) {
        for (int p = i + 1; p < j; ++p) {
            delta += inst.d[tour[p + 1]][tour[p]]
                   - inst.d[tour[p]][tour[p + 1]];
        }
    }
    return delta;
}

long long moveDelta(const Instance& inst, const std::vector<int>& tour,
                    const std::vector<int>& unselected, const Move& move) {
    switch (move.type) {
        case InterExchange:
            return interDelta(inst, tour, move.i, unselected[move.j]);
        case NodeSwap:
            return nodeSwapDelta(inst, tour, move.i, move.j);
        default:
            return edgeSwapDelta(inst, tour, move.i, move.j);
    }
}

void applyMove(std::vector<int>& tour, std::vector<char>& selected,
               std::vector<int>& unselected, const Move& move) {
    if (move.type == InterExchange) {
        const int oldNode = tour[move.i];
        const int newNode = unselected[move.j];
        selected[oldNode] = 0;
        selected[newNode] = 1;
        tour[move.i] = newNode;
        unselected[move.j] = oldNode;
    } else if (move.type == NodeSwap) {
        std::swap(tour[move.i], tour[move.j]);
    } else {
        std::reverse(tour.begin() + move.i + 1, tour.begin() + move.j + 1);
    }
}

void appendMoves(int k, int numUnselected, IntraMove intra,
                 std::vector<Move>& moves) {
    // Inter-route: one selected node (a position) with one slot in the
    // unselected-node list. After an exchange the list entry is swapped, so the
    // descriptor (position, slot) remains valid.
    for (int pos = 0; pos < k; ++pos) {
        for (int slot = 0; slot < numUnselected; ++slot) {
            moves.push_back({InterExchange, pos, slot});
        }
    }

    if (intra == IntraMove::Nodes) {
        for (int i = 0; i < k; ++i) {
            for (int j = i + 1; j < k; ++j) {
                moves.push_back({NodeSwap, i, j});
            }
        }
    } else {
        for (int i = 0; i < k; ++i) {
            for (int j = i + 2; j < k; ++j) {
                // (0,1) and (k-1,0) share node 0: not a valid 2-opt pair.
                if (i == 0 && j == k - 1) continue;
                moves.push_back({EdgeSwap, i, j});
            }
        }
    }
}

std::vector<int> localSearch(const Instance& inst, std::vector<int> tour,
                             SearchMode mode, IntraMove intra,
                             std::mt19937& rng, BuildStats& stats) {
    const int k = static_cast<int>(tour.size());
    const Solution start = evaluate(inst, tour);
    long long value = start.value;
    stats.initialValue = value;

    std::vector<char> selected(inst.n, 0);
    for (int v : tour) selected[v] = 1;

    std::vector<int> unselected;
    unselected.reserve(inst.n - k);
    for (int v = 0; v < inst.n; ++v)
        if (!selected[v]) unselected.push_back(v);

    const int intraCount = (intra == IntraMove::Nodes)
        ? k * (k - 1) / 2
        : k * (k - 3) / 2;
    std::vector<Move> moves;
    moves.reserve(static_cast<std::size_t>(k) * (inst.n - k) + intraCount);
    appendMoves(k, static_cast<int>(unselected.size()), intra, moves);

    if (mode == SearchMode::Steepest) {
        while (true) {
            long long bestDelta = 0;
            Move bestMove{0, 0, 0};
            bool found = false;

            for (const Move& move : moves) {
                const long long delta = moveDelta(inst, tour, unselected, move);
                ++stats.evaluations;
                if (delta >= 0) continue;
                if (delta < bestDelta) {
                    bestDelta = delta;
                    bestMove = move;
                    found = true;
                }
            }

            if (!found) break;
            applyMove(tour, selected, unselected, bestMove);
            value += bestDelta;
            ++stats.moves;
        }
    } else {
        // Greedy: lazy Fisher-Yates shuffle. Only the descriptors that are
        // actually visited are swapped into the random prefix, so the cost per
        // iteration is proportional to the number of evaluated deltas rather
        // than to the size of the whole neighbourhood.
        const std::size_t count = moves.size();
        while (true) {
            bool improved = false;

            for (std::size_t idx = 0; idx < count; ++idx) {
                std::uniform_int_distribution<std::size_t> pick(idx, count - 1);
                std::swap(moves[idx], moves[pick(rng)]);

                const Move& move = moves[idx];
                const long long delta = moveDelta(inst, tour, unselected, move);
                ++stats.evaluations;
                if (delta >= 0) continue;

                applyMove(tour, selected, unselected, move);
                value += delta;
                ++stats.moves;
                improved = true;
                break;
            }

            if (!improved) break;
        }
    }

    const Solution check = evaluate(inst, tour);
    if (check.value != value)
        throw std::runtime_error("local search delta accumulation mismatch");
    return tour;
}

std::vector<int> buildLocalSearch(BuildContext& ctx, SearchMode mode,
                                  IntraMove intra, StartMode startMode) {
    std::vector<int> tour = (startMode == StartMode::Random)
        ? lab1::randomTour(ctx.inst, ctx.rng)
        : lab1::greedyCycle(ctx.inst, ctx.start);
    return localSearch(ctx.inst, std::move(tour), mode, intra, ctx.rng, ctx.stats);
}

struct Variant {
    const char* key;
    const char* label;
    SearchMode mode;
    IntraMove intra;
    StartMode start;
};

}  // namespace

void registerMethods(std::vector<Method>& methods) {
    static const Variant variants[] = {
        {"ls_steepest_nodes_random", "Steepest, node swap, random start",
         SearchMode::Steepest, IntraMove::Nodes, StartMode::Random},
        {"ls_steepest_edges_random", "Steepest, edge swap, random start",
         SearchMode::Steepest, IntraMove::Edges, StartMode::Random},
        {"ls_steepest_nodes_greedy", "Steepest, node swap, greedy start",
         SearchMode::Steepest, IntraMove::Nodes, StartMode::Greedy},
        {"ls_steepest_edges_greedy", "Steepest, edge swap, greedy start",
         SearchMode::Steepest, IntraMove::Edges, StartMode::Greedy},
        {"ls_greedy_nodes_random", "Greedy, node swap, random start",
         SearchMode::Greedy, IntraMove::Nodes, StartMode::Random},
        {"ls_greedy_edges_random", "Greedy, edge swap, random start",
         SearchMode::Greedy, IntraMove::Edges, StartMode::Random},
        {"ls_greedy_nodes_greedy", "Greedy, node swap, greedy start",
         SearchMode::Greedy, IntraMove::Nodes, StartMode::Greedy},
        {"ls_greedy_edges_greedy", "Greedy, edge swap, greedy start",
         SearchMode::Greedy, IntraMove::Edges, StartMode::Greedy},
    };

    for (const Variant& variant : variants) {
        const SearchMode mode = variant.mode;
        const IntraMove intra = variant.intra;
        const StartMode start = variant.start;
        methods.push_back({
            variant.key, variant.label, 3, start == StartMode::Greedy,
            [mode, intra, start](BuildContext& ctx) {
                return buildLocalSearch(ctx, mode, intra, start);
            }
        });
    }
}

}  // namespace lab3
