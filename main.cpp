#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <future>
#include <chrono>

#include "thread_pool.hpp"
#include "searcher.hpp"
#include "dir_walker.hpp"

// ─── How the whole thing fits together ───────────────────────────────────────
//
//   main()
//    ├─ parse CLI args  (pattern, path, flags)
//    ├─ collect_files() → list of all file paths
//    ├─ ThreadPool      → N worker threads (default: hardware_concurrency)
//    │    └─ for each file: submit(search_file) → returns std::future<vector<Match>>
//    └─ collect all futures → print results
//
// The key insight: we dispatch all files at once, workers run in parallel,
// and we wait on futures only at the end. This is the classic fan-out pattern.
// ─────────────────────────────────────────────────────────────────────────────

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [OPTIONS] PATTERN PATH\n"
              << "\nOptions:\n"
              << "  -i            Case-insensitive matching\n"
              << "  -n            Print line numbers (default: on)\n"
              << "  -t <threads>  Number of threads (default: hardware concurrency)\n"
              << "  -h            Show this help\n"
              << "\nExamples:\n"
              << "  " << prog << " 'TODO' ./src\n"
              << "  " << prog << " -i 'error' /var/log\n"
              << "  " << prog << " -t 8 'main' ./project\n";
}

int main(int argc, char* argv[])
{
    // ── 1. Parse arguments ────────────────────────────────────────────────
    if (argc < 3) { print_usage(argv[0]); return 1; }

    bool        case_insensitive = false;
    bool        show_line_nums   = true;
    int         num_threads      = static_cast<int>(std::thread::hardware_concurrency());
    std::string pattern_str;
    std::string search_path;

    // Simple manual arg parsing (no getopt dependency)
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i") {
            case_insensitive = true;
        } else if (arg == "-n") {
            show_line_nums = true;
        } else if (arg == "-t" && i + 1 < argc) {
            num_threads = std::stoi(argv[++i]);
        } else if (arg == "-h") {
            print_usage(argv[0]); return 0;
        } else if (pattern_str.empty()) {
            pattern_str = arg;
        } else {
            search_path = arg;
        }
    }

    if (pattern_str.empty() || search_path.empty()) {
        print_usage(argv[0]); return 1;
    }

    // ── 2. Compile the regex once (shared across all threads, read-only) ──
    std::regex::flag_type flags = std::regex::ECMAScript | std::regex::optimize;
    if (case_insensitive) flags |= std::regex::icase;

    std::regex pattern;
    try {
        pattern = std::regex(pattern_str, flags);
    } catch (const std::regex_error& e) {
        std::cerr << "Invalid regex: " << e.what() << "\n";
        return 1;
    }

    // ── 3. Collect files ──────────────────────────────────────────────────
    std::vector<std::string> files;
    try {
        files = collect_files(search_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    if (files.empty()) {
        std::cerr << "No files found in: " << search_path << "\n";
        return 1;
    }

    // ── 4. Fan out: submit each file to the thread pool ───────────────────
    auto t_start = std::chrono::high_resolution_clock::now();

    ThreadPool pool(num_threads);

    // Each future holds the result of one file search
    std::vector<std::pair<std::string, std::future<std::vector<Match>>>> futures;
    futures.reserve(files.size());

    for (auto& filepath : files) {
        // Capture filepath and pattern by value — each task is independent
        futures.emplace_back(
            filepath,
            pool.submit(search_file, filepath, std::cref(pattern))
        );
    }

    // ── 5. Collect results and print ──────────────────────────────────────
    int total_matches = 0;

    for (auto& [filepath, fut] : futures) {
        auto matches = fut.get(); // blocks until this file's search is done
        for (auto& m : matches) {
            ++total_matches;
            if (show_line_nums) {
                std::cout << filepath << ":" << m.line_number << ": " << m.line << "\n";
            } else {
                std::cout << filepath << ": " << m.line << "\n";
            }
        }
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

    std::cerr << "\n[searchd] " << total_matches << " match(es) in "
              << files.size() << " file(s) | "
              << num_threads << " thread(s) | "
              << elapsed_ms << " ms\n";

    return (total_matches > 0) ? 0 : 1;
}
