#include "core/common.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string fixed1(double v) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(1) << v;
    return o.str();
}

void writeFile(const fs::path& p, const std::string& content) {
    if (!p.parent_path().empty()) fs::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write file: " + p.string());
    f << content;
}

// ---------------------------------------------------------------------------
//  Reading an instance (fixed format: x;y;cost)
// ---------------------------------------------------------------------------
Instance loadInstance(const fs::path& path) {
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
