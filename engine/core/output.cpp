#include "core/output.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string fixed1(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

namespace {

void writeFile(const fs::path& path, const std::string& content) {
    if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot write file: " + path.string());
    file << content;
}

}  // namespace

void writeResults(const fs::path& outDir,
                  const std::vector<InstanceOutput>& outputs, int runs) {
    std::ostringstream summaryCsv, runsCsv, toursCsv;
    std::vector<std::string> columnNames;
    std::vector<std::vector<int>> columnTours;

    summaryCsv << "instance,method,min,max,average,best,first_node,time_ms\n";
    runsCsv << "instance,method,run,start_node,objective,distance,costs,"
               "initial_objective,moves,evaluated\n";
    toursCsv << "instance,method,objective,distance,costs,first_node,tour\n";

    for (const InstanceOutput& output : outputs) {
        for (const MethodResult& result : output.methods) {
            summaryCsv << output.instance << ',' << result.key << ','
                       << result.minV << ',' << result.maxV << ','
                       << fixed1(result.avgV) << ',' << result.best().value << ','
                       << result.best().tour.front() << ','
                       << fixed1(result.ms) << '\n';

            for (int r = 0; r < runs; ++r) {
                const Solution& s = result.sols[r];
                runsCsv << output.instance << ',' << result.key << ',' << r << ','
                        << result.startNode[r] << ',' << s.value << ',' << s.dist
                        << ',' << s.cost << ',' << result.initialValue[r] << ','
                        << result.moves[r] << ',' << result.evaluations[r] << '\n';
            }

            const Solution& best = result.best();
            toursCsv << output.instance << ',' << result.key << ','
                     << best.value << ',' << best.dist << ',' << best.cost << ','
                     << best.tour.front() << ",\"";
            for (std::size_t i = 0; i < best.tour.size(); ++i) {
                if (i) toursCsv << ' ';
                toursCsv << best.tour[i];
            }
            toursCsv << "\"\n";

            columnNames.push_back(output.instance + "_" + result.key);
            columnTours.push_back(best.tour);
        }
    }

    std::ostringstream columnsCsv;
    for (std::size_t c = 0; c < columnNames.size(); ++c) {
        if (c) columnsCsv << ',';
        columnsCsv << columnNames[c];
    }
    columnsCsv << '\n';

    if (!columnTours.empty()) {
        for (std::size_t i = 0; i < columnTours.front().size(); ++i) {
            for (std::size_t c = 0; c < columnTours.size(); ++c) {
                if (c) columnsCsv << ',';
                columnsCsv << columnTours[c][i];
            }
            columnsCsv << '\n';
        }
    }

    writeFile(outDir / "summary.csv", summaryCsv.str());
    writeFile(outDir / "runs.csv", runsCsv.str());
    writeFile(outDir / "best_tours.csv", toursCsv.str());
    writeFile(outDir / "best_tours_columns.csv", columnsCsv.str());
}
