#pragma once

#include <string>

struct Options {
    std::string outDir = "results";
    std::string dataDir = "data";
    unsigned seed = 12345;
    int runs = 200;
    double wr = 0.5;
    double wb = 0.5;
    int candidates = 10;  // candidate-list size for the Lab-4 methods
    int labMask = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
    std::string only;   // optional method-key filter
};

Options parseOptions(int argc, char** argv);
