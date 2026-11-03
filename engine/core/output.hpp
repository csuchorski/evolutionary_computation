#pragma once

#include "core/driver.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct InstanceOutput {
    std::string instance;
    std::vector<MethodResult> methods;
};

// Writes summary.csv, runs.csv, best_tours.csv and best_tours_columns.csv.
void writeResults(const fs::path& outDir,
                  const std::vector<InstanceOutput>& outputs, int runs);

std::string fixed1(double value);
