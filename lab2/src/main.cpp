#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/common.hpp"
#include "lab1/lab1.hpp"
#include "lab2/lab2.hpp"

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

static MethodResult runMethod(const Instance& inst, const Method& method, int runs,
                              std::mt19937& rng, double wr, double wb) {
    MethodResult res;
    res.key = method.key;
    res.label = method.label;
    res.sols.reserve(runs);
    res.startNode.reserve(runs);

    const auto t0 = std::chrono::steady_clock::now();
    for (int r = 0; r < runs; ++r) {
        BuildContext ctx{inst, r % inst.n, rng, wr, wb};   // one start per node
        std::vector<int> tour = method.build(ctx);
        Solution s = evaluate(inst, std::move(tour));
        validate(inst, s);
        res.startNode.push_back(method.usesStart ? ctx.start : s.tour.front());
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

// --lab accepts "all", a single lab ("1", "2") or a comma-separated list
// ("1,2"). Bit `lab` of the returned mask is set for every selected lab.
static int parseLabMask(const std::string& value) {
    if (value == "all") return (1 << 1) | (1 << 2);
    int mask = 0;
    std::stringstream ss(value);
    std::string part;
    while (std::getline(ss, part, ',')) {
        if (part.empty()) continue;
        const int lab = std::stoi(part);
        if (lab < 1 || lab > 2) throw std::runtime_error("unknown lab " + part);
        mask |= (1 << lab);
    }
    if (mask == 0) throw std::runtime_error("empty --lab value");
    return mask;
}

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    try {
        std::string outDir = "results";
        unsigned seed = 12345;
        int runs = 200;
        double wr = 0.5, wb = 0.5;
        int labMask = (1 << 1) | (1 << 2);   // all labs by default

        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto next = [&](const char* what) -> std::string {
                if (i + 1 >= argc) throw std::runtime_error(std::string("missing value for ") + what);
                return argv[++i];
            };
            if (a == "--out") outDir = next("--out");
            else if (a == "--seed") seed = static_cast<unsigned>(std::stoul(next("--seed")));
            else if (a == "--runs") runs = std::stoi(next("--runs"));
            else if (a == "--wr") wr = std::stod(next("--wr"));
            else if (a == "--wb") wb = std::stod(next("--wb"));
            else if (a == "--lab") labMask = parseLabMask(next("--lab"));
            else throw std::runtime_error("unknown option " + a);
        }

        // Collect the methods of every requested assignment, in registration order.
        std::vector<Method> registry;
        registerLab1(registry);
        registerLab2(registry);

        std::vector<Method> methods;
        for (const Method& m : registry) {
            if (labMask & (1 << m.lab)) methods.push_back(m);
        }
        if (methods.empty()) throw std::runtime_error("no methods selected by --lab");

        fs::create_directories(outDir);
        std::mt19937 rng(seed);

        const std::vector<std::string> files = {"TSPA.csv", "TSPB.csv"};

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

            for (const auto& method : methods) {
                const MethodResult mr = runMethod(inst, method, runs, rng, wr, wb);

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
