#pragma once

#include <filesystem>
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
    bool symmetric = true;            // d(i,j) == d(j,i) for the loaded instance
    std::vector<long long> cost;
    std::vector<std::vector<int>> d;  // integer distance matrix
};

Instance loadInstance(const fs::path& path);
