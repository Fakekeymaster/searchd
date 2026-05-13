#pragma once

#include <string>
#include <vector>
#include <regex>

// A single match: line number + the matching line text
struct Match {
    int         line_number;
    std::string line;
};

// Search a file for lines matching the regex pattern.
// Uses mmap for zero-copy I/O (see searcher.cpp for details).
std::vector<Match> search_file(const std::string& filepath,
                               const std::regex&  pattern);
