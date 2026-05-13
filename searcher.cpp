#include "searcher.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <string_view>

using namespace std;

// How mmap works (simple explanation):
//   Normally: open file → read() copies bytes from kernel → your buffer (2 copies)
//   With mmap: the OS maps the file pages directly into your process address space.
//              You access it like a char array — zero extra copies.
//   This is the same trick used by grep, ripgrep, databases, etc.

std::vector<Match> search_file(const std::string& filepath,
                               const std::regex&  pattern)
{
    std::vector<Match> results;

    // 1. Open the file
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) return results; // skip unreadable files silently

    // 2. Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1 || sb.st_size == 0) {
        close(fd);
        return results;
    }

    size_t file_size = static_cast<size_t>(sb.st_size);

    // 3. mmap: map the whole file into memory (read-only, private mapping)
    //    MAP_PRIVATE means writes (if any) won't affect the real file
    void* mapped = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd); // fd no longer needed after mmap

    if (mapped == MAP_FAILED) return results;

    // Hint to the OS: we'll scan this sequentially, please prefetch pages
    madvise(mapped, file_size, MADV_SEQUENTIAL);

    // 4. Walk line by line using string_view — no copies, just pointer arithmetic
    const char* begin = static_cast<const char*>(mapped);
    const char* end   = begin + file_size;
    const char* line_start = begin;
    int line_num = 1;

    for (const char* p = begin; p <= end; ++p) {
        if (p == end || *p == '\n') {
            std::string_view line(line_start, p - line_start);

            // regex_search on the line
            // std::regex requires a string/iterators, so we construct one
            // (string_view iterators work in C++17)
            if (std::regex_search(line.begin(), line.end(), pattern)) {
                results.push_back({ line_num, std::string(line) });
            }

            line_start = p + 1;
            ++line_num;
        }
    }

    munmap(mapped, file_size);
    return results;
}
