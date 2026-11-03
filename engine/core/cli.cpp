#include "core/cli.hpp"

#include <sstream>
#include <stdexcept>

namespace {

// --lab accepts "all", a single lab ("1", "2", "3") or a comma-separated list
// ("1,2,3"). Bit `lab` of the returned mask is set for every selected lab.
int parseLabMask(const std::string& value) {
    if (value == "all") return (1 << 1) | (1 << 2) | (1 << 3);
    int mask = 0;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, ',')) {
        if (part.empty()) continue;
        const int lab = std::stoi(part);
        if (lab < 1 || lab > 3) throw std::runtime_error("unknown lab " + part);
        mask |= (1 << lab);
    }
    if (mask == 0) throw std::runtime_error("empty --lab value");
    return mask;
}

}  // namespace

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&](const char* what) -> std::string {
            if (i + 1 >= argc)
                throw std::runtime_error(std::string("missing value for ") + what);
            return argv[++i];
        };
        if (arg == "--out") options.outDir = next("--out");
        else if (arg == "--data") options.dataDir = next("--data");
        else if (arg == "--seed")
            options.seed = static_cast<unsigned>(std::stoul(next("--seed")));
        else if (arg == "--runs") options.runs = std::stoi(next("--runs"));
        else if (arg == "--wr") options.wr = std::stod(next("--wr"));
        else if (arg == "--wb") options.wb = std::stod(next("--wb"));
        else if (arg == "--lab") options.labMask = parseLabMask(next("--lab"));
        else throw std::runtime_error("unknown option " + arg);
    }
    if (options.runs <= 0) throw std::runtime_error("--runs must be positive");
    return options;
}
