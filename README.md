# searchd — Multithreaded File Search CLI in C++17

A `grep`-like command-line search tool built to demonstrate **C++17 systems programming** concepts:
- `mmap`-based zero-copy I/O
- Custom thread pool using `std::mutex`, `std::condition_variable`, and `std::packaged_task`
- Recursive directory traversal with `std::filesystem`
- Fan-out parallelism via `std::future` / `std::promise`

No OpenMP. No external concurrency libraries. Just standard C++17 and POSIX.

---

## Architecture

```
main()
 ├── parse CLI args
 ├── collect_files()       — recursive std::filesystem walk
 ├── ThreadPool(N)         — N worker threads blocking on condition_variable
 │    └── submit(search_file, filepath, pattern)
 │         └── returns std::future<vector<Match>>
 └── for each future → fut.get() → print matches
```

**Three components:**

| File | Responsibility |
|------|---------------|
| `thread_pool.hpp` | Worker thread pool — mutex + condition_variable + packaged_task |
| `searcher.cpp` | Per-file search — mmap zero-copy I/O + regex matching |
| `dir_walker.cpp` | Recursive directory traversal via std::filesystem |
| `main.cpp` | CLI parsing, fan-out dispatch, result collection + timing |

---

## How mmap works (zero-copy I/O)

Normally when you `read()` a file:
```
Disk → kernel buffer → your buffer    (2 copies)
```

With `mmap`, the OS maps file pages directly into your process address space:
```
Disk → your address space             (0 copies)
```

You access the file like a `char*` array. The OS handles page faults lazily — 
only loading pages you actually touch. This is the same technique used by `grep`, 
`ripgrep`, and most databases.

---

## Thread Pool Design

```cpp
// Worker loop (simplified):
while (true) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
    auto task = std::move(tasks_.front()); tasks_.pop();
    lock.unlock();
    task();  // run outside the lock
}
```

- `condition_variable::wait()` — workers sleep with zero CPU until work arrives
- `packaged_task` — wraps any callable and connects it to a `std::future`
- `submit()` returns a `future` so the caller can collect results asynchronously

---

## Build

```bash
# Standard x86_64 build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./searchd 'TODO' ./src
```

### Cross-compile for ARM64
```bash
# Requires: sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
mkdir build-arm64 && cd build-arm64
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
*(Same sysroot pipeline used for ARM64 embedded switch targets in production.)*

---

## Usage

```bash
searchd [OPTIONS] PATTERN PATH

Options:
  -i            Case-insensitive matching
  -n            Print line numbers (default: on)
  -t <threads>  Number of threads (default: hardware concurrency)
  -h            Help

Examples:
  searchd 'TODO' ./src
  searchd -i 'error' /var/log
  searchd -t 8 'panic' /var/log
```

---

## Benchmark

Corpus: **3,000 files** (~18 MB total) on Linux, NVMe SSD, 4-core machine.

| Tool | Threads | Time | Matches |
|------|---------|------|---------|
| GNU grep -r | 1 (no parallelism) | ~21 ms | 7,500 |
| searchd | 1 | ~200 ms | 7,500 |
| searchd | 4 | ~170 ms | 7,500 |

**GNU grep is faster** — it uses highly optimized Boyer-Moore-Horspool string matching 
and kernel-level buffering tuned over decades. `searchd` uses `std::regex` (ECMAScript 
engine) which is significantly heavier per match.

The goal of this project is not to beat grep — it's to demonstrate:
1. How to build a **correct, thread-safe** concurrent search pipeline in pure C++17
2. How `mmap`, `condition_variable`, `packaged_task`, and `future` fit together
3. How to structure a CMake project for both x86_64 and ARM64 targets

---

## License

MIT
