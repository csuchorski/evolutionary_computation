#include "core/instance.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>

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

    // Distance matrix computed once, immediately after reading. Both supplied
    // instances are Euclidean, so the matrix is symmetric by construction.
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
