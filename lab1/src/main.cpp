#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
//  Instance: integer distance matrix and node costs
// ---------------------------------------------------------------------------
struct Instance {
    std::string name;                 // file stem, e.g. "TSPA"
    std::string source;               // file name, e.g. "TSPA.csv"
    int n = 0;                        // number of nodes
    int k = 0;                        // number of selected nodes
    std::vector<long long> cost;
    std::vector<std::vector<int>> d;  // directed integer distance matrix
};

// ---------------------------------------------------------------------------
//  Solution
// ---------------------------------------------------------------------------
struct Solution {
    std::vector<int> tour;   // directed cyclic order, size k
    long long value = 0;     // total objective
    long long dist = 0;      // sum of the k arc lengths (closing arc included)
    long long cost = 0;      // sum of the k node costs
};

static std::string fixed1(double v) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << v;
    return o.str();
}

static void writeFile(const fs::path& p, const std::string& content) {
    if (!p.parent_path().empty()) fs::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write file: " + p.string());
    f << content;
}

// ---------------------------------------------------------------------------
//  Reading an instance (fixed format: x;y;cost)
// ---------------------------------------------------------------------------
static Instance loadInstance(const fs::path& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open instance: " + path.string());

    std::vector<long long> xs, ys;
    Instance inst;
    inst.source = path.filename().string();
    inst.name = path.stem().string();

    std::string line;
    while (std::getline(in, line)) {
        long long x, y, c;
        if (std::sscanf(line.c_str(), "%lld;%lld;%lld", &x, &y, &c) != 3) continue;
        xs.push_back(x);
        ys.push_back(y);
        inst.cost.push_back(c);
    }
    inst.n = static_cast<int>(inst.cost.size());
    inst.k = (inst.n + 1) / 2;

    // distance matrix computed once, immediately after reading
    inst.d.assign(inst.n, std::vector<int>(inst.n, 0));
    for (int i = 0; i < inst.n; ++i) {
        for (int j = i + 1; j < inst.n; ++j) {
            const double dx = static_cast<double>(xs[i] - xs[j]);
            const double dy = static_cast<double>(ys[i] - ys[j]);
            const int v = static_cast<int>(std::llround(std::hypot(dx, dy)));
            inst.d[i][j] = v;
            inst.d[j][i] = v;
        }
    }
    return inst;
}

// ---------------------------------------------------------------------------
//  Objective function: closed directed cycle
// ---------------------------------------------------------------------------
static Solution evaluate(const Instance& inst, std::vector<int> tour) {
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

static void validate(const Instance& inst, const Solution& s) {
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

// ---------------------------------------------------------------------------
//  Running one method
// ---------------------------------------------------------------------------
struct MethodResult {
    std::string key;          // file-friendly identifier
    std::string label;        // human-readable name
    std::vector<Solution> sols;
    std::vector<int> startNode;
    long long minV = 0, maxV = 0;
    double avgV = 0.0;
    int bestRun = 0;
    double ms = 0.0;

    const Solution& best() const { return sols[bestRun]; }
};

static MethodResult runMethod(const Instance& inst, const std::string& key,
                              const std::string& label, int runs,
                              std::mt19937& rng) {
    MethodResult res;
    res.key = key;
    res.label = label;
    res.sols.reserve(runs);
    res.startNode.reserve(runs);

    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < runs; ++r) {
        std::vector<int> tour;
        int start = -1;
        if (key == "random") {
            tour = randomTour(inst, rng);
        } else {
            start = r % inst.n;   // one start per node when runs == n
            if (key == "nn_end") tour = nnAppend(inst, start);
            else if (key == "nn_all") tour = nnAnyPosition(inst, start);
            else if (key == "greedy_cycle") tour = greedyCycle(inst, start);
            else throw std::runtime_error("unknown method " + key);
        }
        Solution s = evaluate(inst, std::move(tour));
        validate(inst, s);
        if (start < 0) start = s.tour.front();
        res.startNode.push_back(start);
        res.sols.push_back(std::move(s));
    }
    const auto t1 = std::chrono::steady_clock::now();
    res.ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    res.minV = res.maxV = res.sols[0].value;
    double sum = 0.0;
    for (int r = 0; r < runs; ++r) {
        res.minV = std::min(res.minV, res.sols[r].value);
        res.maxV = std::max(res.maxV, res.sols[r].value);
        sum += static_cast<double>(res.sols[r].value);
        if (res.sols[r].value < res.sols[res.bestRun].value) res.bestRun = r;
    }
    res.avgV = sum / runs;
    return res;
}

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    try {
        std::string outDir = "results";
        unsigned seed = 12345;
        int runs = 200;

        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto next = [&](const char* what) -> std::string {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + what);
                return argv[++i];
            };
            if (a == "--out") outDir = next("--out");
            else if (a == "--seed") seed = static_cast<unsigned>(std::stoul(next("--seed")));
            else if (a == "--runs") runs = std::stoi(next("--runs"));
            else throw std::runtime_error("unknown option " + a);
        }

        fs::create_directories(outDir);
        std::mt19937 rng(seed);

        const std::vector<std::string> files = {"TSPA.csv", "TSPB.csv"};
        const std::vector<std::pair<std::string, std::string>> methodDefs = {
            {"random", "Random"},
            {"nn_end", "Nearest neighbour (append at end)"},
            {"nn_all", "Nearest neighbour (any position)"},
            {"greedy_cycle", "Greedy cycle"}
        };

        std::ostringstream summaryCsv, runsCsv, toursCsv;
        std::vector<std::string> columnNames;
        std::vector<std::vector<int>> columnTours;
        summaryCsv << "instance,method,min,max,average,best,first_node,time_ms\n";
        runsCsv << "instance,method,run,start_node,objective,distance,costs\n";
        toursCsv << "instance,method,objective,distance,costs,first_node,tour\n";

        for (const auto& file : files) {
            const Instance inst = loadInstance(file);
            std::cout << "Instance " << inst.name << ": n = " << inst.n
                      << ", k = " << inst.k << "\n";

            for (const auto& def : methodDefs) {
                const MethodResult mr = runMethod(inst, def.first, def.second, runs, rng);

                std::cout << "  " << std::setw(28) << std::left << mr.label
                          << " min " << std::setw(8) << mr.minV
                          << " max " << std::setw(8) << mr.maxV
                          << " avg " << std::setw(10) << fixed1(mr.avgV)
                          << "  (" << fixed1(mr.ms) << " ms)\n";

                summaryCsv << inst.name << ',' << mr.key << ',' << mr.minV << ',' << mr.maxV
                           << ',' << fixed1(mr.avgV) << ',' << mr.best().value << ','
                           << mr.best().tour.front() << ',' << fixed1(mr.ms) << '\n';

                for (int r = 0; r < runs; ++r) {
                    const Solution& s = mr.sols[r];
                    runsCsv << inst.name << ',' << mr.key << ',' << r << ','
                            << mr.startNode[r] << ',' << s.value << ',' << s.dist
                            << ',' << s.cost << '\n';
                }

                const Solution& b = mr.best();
                toursCsv << inst.name << ',' << mr.key << ',' << b.value << ',' << b.dist
                         << ',' << b.cost << ',' << b.tour.front() << ",\"";
                for (std::size_t i = 0; i < b.tour.size(); ++i) {
                    if (i) toursCsv << ' ';
                    toursCsv << b.tour[i];
                }
                toursCsv << "\"\n";
                columnNames.push_back(inst.name + "_" + mr.key);
                columnTours.push_back(b.tour);
            }
        }

        std::ostringstream columnsCsv;
        for (std::size_t c = 0; c < columnNames.size(); ++c) {
            if (c) columnsCsv << ',';
            columnsCsv << columnNames[c];
        }
        columnsCsv << '\n';
        for (std::size_t i = 0; i < columnTours.front().size(); ++i) {
            for (std::size_t c = 0; c < columnTours.size(); ++c) {
                if (c) columnsCsv << ',';
                columnsCsv << columnTours[c][i];
            }
            columnsCsv << '\n';
        }

        writeFile(fs::path(outDir) / "summary.csv", summaryCsv.str());
        writeFile(fs::path(outDir) / "runs.csv", runsCsv.str());
        writeFile(fs::path(outDir) / "best_tours.csv", toursCsv.str());
        writeFile(fs::path(outDir) / "best_tours_columns.csv", columnsCsv.str());

        std::cout << "\nResults written to " << outDir << "/\n"
                  << "  summary.csv, runs.csv, best_tours.csv, "
                     "best_tours_columns.csv\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
