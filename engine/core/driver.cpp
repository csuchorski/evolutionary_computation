#include "core/driver.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace {

uint32_t fnv1a(const std::string& text) {
    uint32_t hash = 2166136261u;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

}  // namespace

std::mt19937 makeMethodRng(unsigned baseSeed, const std::string& key) {
    std::seed_seq seq{baseSeed, fnv1a(key), 0x9e3779b9u};
    return std::mt19937(seq);
}

MethodResult runMethod(const Instance& inst, const Method& method, int runs,
                       unsigned seed, double wr, double wb) {
    MethodResult res;
    res.key = method.key;
    res.label = method.label;
    res.sols.reserve(runs);
    res.startNode.reserve(runs);
    res.initialValue.reserve(runs);
    res.moves.reserve(runs);
    res.evaluations.reserve(runs);

    std::mt19937 rng = makeMethodRng(seed, method.key);

    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < runs; ++r) {
        BuildContext ctx{inst, r % inst.n, rng, wr, wb, {}};   // one start per node
        std::vector<int> tour = method.build(ctx);
        Solution s = evaluate(inst, std::move(tour));
        validate(inst, s);

        const long long initial = ctx.stats.initialValue != 0
            ? ctx.stats.initialValue : s.value;
        if (ctx.stats.initialValue != 0 && s.value > ctx.stats.initialValue)
            throw std::runtime_error("local search made the solution worse");

        res.startNode.push_back(method.usesStart ? ctx.start : s.tour.front());
        res.initialValue.push_back(initial);
        res.moves.push_back(ctx.stats.moves);
        res.evaluations.push_back(ctx.stats.evaluations);
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
