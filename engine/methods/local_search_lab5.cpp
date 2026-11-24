#include "methods/local_search_lab5.hpp"

#include "core/objective.hpp"
#include "methods/heuristics_lab1.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lab5 {
namespace {

// ---------------------------------------------------------------------------
//  Candidate lists for the optional LM + candidate method
//
//  The lists are rebuilt for every run so that their construction is part of
//  the measured running time.
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
            const long long key = inst.d[i][j] + (withCost ? inst.cost[j] : 0);
            ranked.emplace_back(key, j);
        }
        std::partial_sort(ranked.begin(), ranked.begin() + take, ranked.end());

        list[i].reserve(take);
        for (int t = 0; t < take; ++t) list[i].push_back(ranked[t].second);
    }
    return list;
}

struct CandidateSet {
    std::string name;
    int count = -1;
    std::vector<std::vector<int>> inter;
    std::vector<std::vector<int>> intra;
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
//  Saved moves
//
//  Inter-route:
//      a = p, b = u, c = n, d = v
//      remove edges (p,u) and (u,n), replace selected u by unselected v.
//      A saved inter move is applicable only while both removed edges are
//      still present (in either direction), so its saved delta stays exact.
//
//  Edge exchange:
//      a->b and c->d are the directed removed edges; add a->c and b->d.
//      (a,b),(c,d) and (b,a),(d,c) are the same saved move ("both reversed").
// ---------------------------------------------------------------------------
enum class MoveType { Inter, Edge };

struct SavedMove {
    MoveType type;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    long long delta = 0;
};

struct MoveKey {
    int type = 0;
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int parity = 0;

    bool operator==(const MoveKey& other) const {
        return type == other.type && a == other.a && b == other.b &&
               c == other.c && d == other.d && parity == other.parity;
    }
};

struct MoveKeyHash {
    std::size_t operator()(const MoveKey& key) const {
        std::size_t h = static_cast<std::size_t>(key.type);
        auto mix = [&h](int value) {
            h ^= static_cast<std::size_t>(value) + 0x9e3779b97f4a7c15ULL +
                 (h << 6) + (h >> 2);
        };
        mix(key.a);
        mix(key.b);
        mix(key.c);
        mix(key.d);
        mix(key.parity);
        return h;
    }
};

MoveKey keyOf(const SavedMove& move) {
    if (move.type == MoveType::Inter) {
        return {0, move.a, move.b, move.c, move.d, 0};
    }

    int lo1 = std::min(move.a, move.b);
    int hi1 = std::max(move.a, move.b);
    int o1 = move.a > move.b ? 1 : 0;
    int lo2 = std::min(move.c, move.d);
    int hi2 = std::max(move.c, move.d);
    int o2 = move.c > move.d ? 1 : 0;

    if (lo1 > lo2 || (lo1 == lo2 && hi1 > hi2)) {
        std::swap(lo1, lo2);
        std::swap(hi1, hi2);
        std::swap(o1, o2);
    }
    return {1, lo1, hi1, lo2, hi2, o1 ^ o2};
}

using KeySet = std::unordered_set<MoveKey, MoveKeyHash>;

struct MovePool {
    std::vector<SavedMove>& lm;
    KeySet* keys;   // nullptr for the full neighbourhood, which has no duplicates
    BuildStats& stats;

    bool add(const SavedMove& move) {
        if (move.delta >= 0) return false;
        if (keys != nullptr) {
            const MoveKey key = keyOf(move);
            if (!keys->insert(key).second) return false;
        }
        lm.push_back(move);
        return true;
    }
};

// ---------------------------------------------------------------------------
//  Deltas
// ---------------------------------------------------------------------------
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

long long edgeDeltaEndpoints(const Instance& inst, int a, int b, int c, int d) {
    return inst.d[a][c] + inst.d[b][d]
         - inst.d[a][b] - inst.d[c][d];
}

bool contains(const std::vector<int>& list, int node) {
    return std::find(list.begin(), list.end(), node) != list.end();
}

bool validEdgePair(int p, int q, int k) {
    if (p == q) return false;
    if ((p + 1) % k == q) return false;
    if ((q + 1) % k == p) return false;
    return true;
}

// +1 if the tour contains x->y, -1 if it contains y->x, 0 if the edge is absent.
int edgeDirection(const std::vector<int>& tour, const std::vector<int>& posOf,
                  int x, int y) {
    const int px = posOf[x];
    if (px < 0 || posOf[y] < 0) return 0;
    const int k = static_cast<int>(tour.size());
    if (tour[(px + 1) % k] == y) return 1;
    if (tour[(px - 1 + k) % k] == y) return -1;
    return 0;
}

// ---------------------------------------------------------------------------
//  Full-neighbourhood enumeration
//
//  Every edge pair is evaluated in both relative directions. Variant B is
//  stored even when it is not applicable now; it may become applicable after a
//  future 2-opt reversal.
// ---------------------------------------------------------------------------
long long enumerateFull(const Instance& inst, const std::vector<int>& tour,
                        const std::vector<char>& selected,
                        const std::vector<int>&, MovePool& pool) {
    const int k = static_cast<int>(tour.size());
    const int n = inst.n;
    long long activeAdded = 0;

    for (int pos = 0; pos < k; ++pos) {
        const int p = tour[(pos - 1 + k) % k];
        const int u = tour[pos];
        const int next = tour[(pos + 1) % k];
        for (int v = 0; v < n; ++v) {
            if (selected[v]) continue;
            ++pool.stats.evaluations;
            const long long delta = interDelta(inst, tour, pos, v);
            if (delta < 0 &&
                pool.add({MoveType::Inter, p, u, next, v, delta})) {
                ++activeAdded;
            }
        }
    }

    for (int i = 0; i < k; ++i) {
        const int a = tour[i];
        const int b = tour[(i + 1) % k];
        for (int j = i + 2; j < k; ++j) {
            if (i == 0 && j == k - 1) continue;
            const int c = tour[j];
            const int e = tour[(j + 1) % k];

            ++pool.stats.evaluations;
            const long long deltaA = edgeDeltaEndpoints(inst, a, b, c, e);
            if (deltaA < 0 &&
                pool.add({MoveType::Edge, a, b, c, e, deltaA})) {
                ++activeAdded;
            }

            ++pool.stats.evaluations;
            const long long deltaB = edgeDeltaEndpoints(inst, a, b, e, c);
            if (deltaB < 0) {
                pool.add({MoveType::Edge, a, b, e, c, deltaB});
            }
        }
    }
    return activeAdded;
}

// ---------------------------------------------------------------------------
//  Candidate-move enumeration for the optional combined method
//
//  In addition to candidate edges stored in the selected endpoint's list, the
//  inter-route scan also considers candidate edges stored only in the
//  unselected endpoint's list. This makes the candidate relation effectively
//  undirected for node exchanges, which recovers much of the baseline quality.
// ---------------------------------------------------------------------------
long long enumerateCandidate(const Instance& inst, const std::vector<int>& tour,
                             const std::vector<char>& selected,
                             const std::vector<int>& posOf,
                             const CandidateSet& set, MovePool& pool) {
    const int k = static_cast<int>(tour.size());
    const int n = inst.n;
    long long activeAdded = 0;

    auto addEdgePair = [&](int p, int q) {
        if (!validEdgePair(p, q, k)) return;
        const int a = tour[p];
        const int b = tour[(p + 1) % k];
        const int c = tour[q];
        const int e = tour[(q + 1) % k];

        ++pool.stats.evaluations;
        const long long deltaA = edgeDeltaEndpoints(inst, a, b, c, e);
        if (deltaA < 0 && pool.add({MoveType::Edge, a, b, c, e, deltaA})) {
            ++activeAdded;
        }

        ++pool.stats.evaluations;
        const long long deltaB = edgeDeltaEndpoints(inst, a, b, e, c);
        if (deltaB < 0) {
            pool.add({MoveType::Edge, a, b, e, c, deltaB});
        }
    };

    // Candidate edges whose selected endpoint is the anchor.
    for (int p = 0; p < k; ++p) {
        const int a = tour[p];

        for (int j : set.intra[a]) {
            if (!selected[j]) continue;
            if (contains(set.intra[j], a) && posOf[j] < p) continue;
            const int q = posOf[j];

            addEdgePair(p, q);

            const int p2 = (p - 1 + k) % k;
            const int q2 = (q - 1 + k) % k;
            addEdgePair(p2, q2);
        }

        for (int j : set.inter[a]) {
            if (selected[j]) continue;

            const int nextPos = (p + 1) % k;
            ++pool.stats.evaluations;
            const long long deltaNext = interDelta(inst, tour, nextPos, j);
            if (deltaNext < 0 &&
                pool.add({MoveType::Inter, a, tour[nextPos],
                          tour[(nextPos + 1) % k], j, deltaNext})) {
                ++activeAdded;
            }

            const int prevPos = (p - 1 + k) % k;
            if (k >= 3 && contains(set.inter[tour[(p - 2 + k) % k]], j))
                continue;
            ++pool.stats.evaluations;
            const long long deltaPrev = interDelta(inst, tour, prevPos, j);
            if (deltaPrev < 0 &&
                pool.add({MoveType::Inter, tour[(prevPos - 1 + k) % k],
                          tour[prevPos], a, j, deltaPrev})) {
                ++activeAdded;
            }
        }
    }

    // Candidate edges stored only in the unselected endpoint's list.
    // Reciprocal pairs were already generated from the selected anchor.
    for (int u = 0; u < n; ++u) {
        if (selected[u]) continue;
        for (int v : set.inter[u]) {
            if (!selected[v]) continue;
            if (contains(set.inter[v], u)) continue;

            const int q = posOf[v];
            const int nextPos = (q + 1) % k;
            ++pool.stats.evaluations;
            const long long deltaNext = interDelta(inst, tour, nextPos, u);
            if (deltaNext < 0 &&
                pool.add({MoveType::Inter, v, tour[nextPos],
                          tour[(nextPos + 1) % k], u, deltaNext})) {
                ++activeAdded;
            }

            const int prevPos = (q - 1 + k) % k;
            ++pool.stats.evaluations;
            const long long deltaPrev = interDelta(inst, tour, prevPos, u);
            if (deltaPrev < 0 &&
                pool.add({MoveType::Inter, tour[(prevPos - 1 + k) % k],
                          tour[prevPos], v, u, deltaPrev})) {
                ++activeAdded;
            }
        }
    }
    return activeAdded;
}

enum class Status { Remove, Skip, Apply };

// ---------------------------------------------------------------------------
//  LM search core
// ---------------------------------------------------------------------------
template <typename Enumerator>
std::vector<int> lmSearch(const Instance& inst, std::vector<int> tour,
                          BuildStats& stats, Enumerator enumerate,
                          bool deduplicate) {
    const int k = static_cast<int>(tour.size());
    const long long startValue = evaluate(inst, tour).value;
    long long value = startValue;
    stats.initialValue = startValue;

    std::vector<char> selected(inst.n, 0);
    std::vector<int> posOf(inst.n, -1);
    for (int p = 0; p < k; ++p) {
        selected[tour[p]] = 1;
        posOf[tour[p]] = p;
    }

    std::vector<SavedMove> lm;
    std::vector<char> removed;
    KeySet keys;
    MovePool pool{lm, deduplicate ? &keys : nullptr, stats};
    std::size_t head = 0;
    bool fresh = true;

    auto rebuild = [&]() {
        ++stats.regenerations;
        lm.clear();
        removed.clear();
        if (deduplicate) keys.clear();
        const long long added = enumerate(inst, tour, selected, posOf, pool);
        removed.assign(lm.size(), 0);
        head = 0;
        stats.maxLmSize =
            std::max<long long>(stats.maxLmSize, static_cast<long long>(lm.size()));
        std::sort(lm.begin(), lm.end(),
                  [](const SavedMove& x, const SavedMove& y) {
                      return x.delta < y.delta;
                  });
        return added;
    };

    if (rebuild() == 0) {
        const Solution check = evaluate(inst, tour);
        if (check.value != value)
            throw std::runtime_error("lab5 LM initial delta mismatch");
        return tour;
    }

    while (true) {
        bool applied = false;

        for (std::size_t idx = head; idx < lm.size(); ++idx) {
            if (removed[idx]) continue;
            ++stats.lmChecks;
            const SavedMove move = lm[idx];

            Status status = Status::Remove;
            if (move.type == MoveType::Inter) {
                if (posOf[move.b] >= 0 && posOf[move.d] < 0) {
                    const int pu = tour[(posOf[move.b] - 1 + k) % k];
                    const int nu = tour[(posOf[move.b] + 1) % k];
                    const bool same = (pu == move.a && nu == move.c) ||
                                      (pu == move.c && nu == move.a);
                    status = same ? Status::Apply : Status::Remove;
                }
            } else {
                const int dir1 = edgeDirection(tour, posOf, move.a, move.b);
                const int dir2 = edgeDirection(tour, posOf, move.c, move.d);
                if (dir1 == 0 || dir2 == 0) status = Status::Remove;
                else if (dir1 != dir2) status = Status::Skip;
                else status = Status::Apply;
            }

            if (status == Status::Skip) {
                ++stats.lmSkipped;
                continue;
            }

            removed[idx] = 1;
            if (status == Status::Remove) {
                ++stats.lmRemoved;
                continue;
            }

            bool ok = true;
            if (move.type == MoveType::Inter) {
                const int pos = posOf[move.b];
                tour[pos] = move.d;
                selected[move.b] = 0;
                selected[move.d] = 1;
                posOf[move.b] = -1;
                posOf[move.d] = pos;
            } else {
                const int dir1 = edgeDirection(tour, posOf, move.a, move.b);
                int p = 0;
                int q = 0;
                if (dir1 == 1) {
                    p = posOf[move.a];
                    q = posOf[move.c];
                } else {
                    p = posOf[move.b];
                    q = posOf[move.d];
                }
                if (!validEdgePair(p, q, k)) {
                    ok = false;
                } else {
                    const int lo = std::min(p, q);
                    const int hi = std::max(p, q);
                    std::reverse(tour.begin() + lo + 1, tour.begin() + hi + 1);
                    for (int t = lo + 1; t <= hi; ++t) posOf[tour[t]] = t;
                }
            }

            if (!ok) {
                ++stats.lmRemoved;
                continue;
            }

            value += move.delta;
            ++stats.moves;
            applied = true;
            break;
        }

        while (head < lm.size() && removed[head]) ++head;

        if (applied) {
            fresh = false;
            continue;
        }
        if (fresh) break;
        if (rebuild() == 0) break;
        fresh = true;
    }

    const Solution check = evaluate(inst, tour);
    if (check.value != value)
        throw std::runtime_error("lab5 LM delta accumulation mismatch");
    return tour;
}

std::vector<int> buildLmFull(BuildContext& ctx) {
    std::vector<int> tour = lab1::randomTour(ctx.inst, ctx.rng);
    return lmSearch(ctx.inst, std::move(tour), ctx.stats,
                    [](const Instance& inst, const std::vector<int>& t,
                       const std::vector<char>& sel, const std::vector<int>& pos,
                       MovePool& pool) {
                        return enumerateFull(inst, t, sel, pos, pool);
                    },
                    false);
}

std::vector<int> buildLmCandidate(BuildContext& ctx) {
    const CandidateSet set = candidateSet(ctx.inst, ctx.candidates);
    std::vector<int> tour = lab1::randomTour(ctx.inst, ctx.rng);
    return lmSearch(ctx.inst, std::move(tour), ctx.stats,
                    [&set](const Instance& inst, const std::vector<int>& t,
                           const std::vector<char>& sel,
                           const std::vector<int>& pos, MovePool& pool) {
                        return enumerateCandidate(inst, t, sel, pos, set, pool);
                    },
                    true);
}

}  // namespace

void registerMethods(std::vector<Method>& methods) {
    methods.push_back({
        "ls_lm_steepest_random",
        "Steepest, LM, random start",
        5, false,
        [](BuildContext& ctx) { return buildLmFull(ctx); },
        "ls_steepest_edges_random"
    });
    methods.push_back({
        "ls_lm_cand_random",
        "Steepest, LM + candidate moves, random start",
        5, false,
        [](BuildContext& ctx) { return buildLmCandidate(ctx); },
        "ls_steepest_edges_random"
    });
}

}  // namespace lab5
