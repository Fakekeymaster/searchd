#include "dir_walker.hpp"

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

// recursive_directory_iterator walks the whole tree for us.
// We skip symlinks to avoid infinite loops (a symlink pointing to a parent dir).
std::vector<std::string> collect_files(const std::string& root_dir)
{
    std::vector<std::string> files;

    if (!fs::exists(root_dir)) {
        throw std::runtime_error("Path does not exist: " + root_dir);
    }

    // If it's a single file, just return it directly
    if (fs::is_regular_file(root_dir)) {
        files.push_back(root_dir);
        return files;
    }

    // Walk the directory tree
    fs::recursive_directory_iterator it(
        root_dir,
        fs::directory_options::skip_permission_denied
    );

    for (auto& entry : it) {
        // Only regular files, skip symlinks and dirs
        if (entry.is_regular_file() && !entry.is_symlink()) {
            files.push_back(entry.path().string());
        }
    }

    return files;
}
