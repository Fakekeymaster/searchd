#pragma once

#include <string>
#include <vector>

// Recursively walk a directory and collect all file paths.
// Uses std::filesystem (C++17) — no manual opendir/readdir needed.
std::vector<std::string> collect_files(const std::string& root_dir);
