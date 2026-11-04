#include "core/cli.hpp"
#include "core/driver.hpp"
#include "core/instance.hpp"
#include "core/output.hpp"
#include "methods/heuristics_lab1.hpp"
#include "methods/local_search_lab3.hpp"
#include "methods/regret_lab2.hpp"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);

        // Collect the methods of every requested assignment, in registration order.
        std::vector<Method> registry;
        lab1::registerMethods(registry);
        lab2::registerMethods(registry);
        lab3::registerMethods(registry);

        std::vector<Method> methods;
        for (const Method& method : registry) {
            if (!(options.labMask & (1 << method.lab))) continue;
            if (!options.only.empty() && method.key != options.only) continue;
            methods.push_back(method);
        }
        if (methods.empty()) throw std::runtime_error("no methods selected by --lab/--only");

        fs::create_directories(options.outDir);

        const std::vector<std::string> files = {"TSPA.csv", "TSPB.csv"};
        std::vector<InstanceOutput> outputs;

        for (const std::string& file : files) {
            const Instance inst = loadInstance(fs::path(options.dataDir) / file);
            std::cout << "Instance " << inst.name << ": n = " << inst.n
                      << ", k = " << inst.k << "\n";

            InstanceOutput output;
            output.instance = inst.name;

            for (const Method& method : methods) {
                MethodResult result = runMethod(inst, method, options.runs,
                                                options.seed, options.wr, options.wb);

                std::cout << "  " << std::setw(34) << std::left << result.label
                          << " min " << std::setw(8) << result.minV
                          << " max " << std::setw(8) << result.maxV
                          << " avg " << std::setw(10) << fixed1(result.avgV)
                          << "  (" << fixed1(result.ms) << " ms)\n";

                output.methods.push_back(std::move(result));
            }
            outputs.push_back(std::move(output));
        }

        writeResults(options.outDir, outputs, options.runs);

        std::cout << "\nResults written to " << options.outDir << "/\n"
                  << "  summary.csv, runs.csv, best_tours.csv, "
                     "best_tours_columns.csv\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
}
