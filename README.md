# shcoro

`shcoro` is a small **C++20 stackless coroutine** library that provides:
- **`Async<T>`**: an awaitable task type for composing coroutines (`co_await`, `co_return`)
- **Schedulers**: a lightweight, **type-erased** scheduler hook to resume coroutines
- **Timer awaiter**: a simple `TimedScheduler` + `TimedAwaiter` (“sleep” / delayed resume)
- **Combinators**: `all_of(...)` / `any_of(...)` to wait on multiple async operations
- **Coroutine-aware mutex**: `MutexLock` with `co_await mutex.lock` + FIFO wakeups
- **Coroutine-aware read write lock**: `RWLock` supporting reader priority and fair policy
- **`Generator<T>`**: a `co_yield` generator that works with range-for

The project builds with CMake and exports a CMake target: **`shcoro::shcoro`**.

### Features

- **Async / task composition**
  - Define coroutine functions returning `shcoro::Async<T>`
  - `co_await` nested `Async` tasks
  - `spawn_async(...)` to run a top-level async task and retrieve its result

- **Pluggable scheduling**
  - `shcoro::Scheduler` is a type-erased wrapper around your scheduler type
  - A scheduler type must provide:
    - `using value_type = ...;`
    - `void register_coro(std::coroutine_handle<>, value_type value);`
    - `void unregister_coro(std::coroutine_handle<>);`

- **Timed scheduling (demo scheduler)**
  - `TimedScheduler` resumes coroutines after a `time_t` delay
  - `co_await TimedAwaiter{seconds};` registers the coroutine into the scheduler

- **Fan-in / multiplexing**
  - `all_of(a, b, c...)`: wait until **all** complete, returns a tuple of results
  - `any_of(a, b, c...)`: wait until **any** completes, returns a variant tagged by index
  - `void` results are represented as `shcoro::empty` in these combinators

- **Coroutine mutex**
  - `lock()` continues coroutine execution if none are waiting or suspends it self until `unlock()` is called 
  - `unlock()` resumes the next waiter or releases the lock if none are waiting

- **Generator**
  - `Generator<T>` supports `co_yield` and range-for iteration

### Notes / current limitations

- **Exceptions**: `Async`’s promise currently uses `std::terminate()` for unhandled exceptions. Catch/handle exceptions inside your coroutine code if you don’t want termination.
- **Timer portability**: `timer.hpp` includes `<sys/time.h>` (POSIX). On Windows, you may need to adjust/includes to build timer demos, or build your own timer.

## Requirements

- **CMake** ≥ 3.2
- **C++20 compiler**
  - GCC/Clang: uses `-fcoroutines`
  - MSVC: uses `/await`

## Build

### Library

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Demos

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSHCORO_BUILD_DEMO=ON
cmake --build build
```

### Tests

Tests require **GTest** available via `find_package(GTest REQUIRED)`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSHCORO_BUILD_TEST=ON
cmake --build build
ctest --test-dir build
```

## Install / Consume

### Install

```bash
cmake --install build --prefix <install-prefix>
```

### Use via `find_package`

```cmake
find_package(shcoro CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE shcoro::shcoro)
```

### Use via `FetchContent` (example)

```cmake
include(FetchContent)

FetchContent_Declare(
  shcoro
  GIT_REPOSITORY <your-repo-url>
  GIT_TAG        <tag-or-commit>
)

FetchContent_MakeAvailable(shcoro)

target_link_libraries(your_target PRIVATE shcoro::shcoro)
```

## Quick examples

### Async composition

```cpp
#include <iostream>
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::spawn_async;

Async<int> inner(int x) {
  co_return x * x;
}

Async<double> middle(int x) {
  int y = co_await inner(x);
  co_return 3.0 * y;
}

int main() {
  auto r = spawn_async(middle(3));
  std::cout << r.get() << "\n";
}
```

### Timed awaiter + scheduler

```cpp
#include <iostream>
#include "shcoro/stackless/timer.hpp"
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::TimedAwaiter;
using shcoro::TimedScheduler;
using shcoro::spawn_async;

Async<int> work(int seconds) {
  co_await TimedAwaiter{seconds};
  co_return seconds * 10;
}

int main() {
  TimedScheduler sched;
  auto a = spawn_async(work(1), sched);
  auto b = spawn_async(work(3), sched);

  sched.run();              // drives the scheduled resumes
  std::cout << a.get() << "\n";
  std::cout << b.get() << "\n";
}
```

### `all_of` / `any_of`

```cpp
#include <tuple>
#include <variant>
#include "shcoro/stackless/timer.hpp"
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::TimedAwaiter;
using shcoro::TimedScheduler;
using shcoro::spawn_async;

Async<int> a() { co_await TimedAwaiter{1}; co_return 10; }
Async<double> b() { co_await TimedAwaiter{2}; co_return 2.5; }
Async<void> c() { co_await TimedAwaiter{3}; co_return; }

Async<long long> demo_all() {
  auto t = co_await shcoro::all_of(a(), b(), c());
  co_return (long long)std::get<0>(t) + (long long)std::get<1>(t);
}

Async<long long> demo_any() {
  auto v = co_await shcoro::any_of(a(), b());
  // v is a variant of indexed results; the demo uses std::get<0>(v).value
  co_return std::get<0>(v).value;
}

int main() {
  TimedScheduler sched;
  auto r = spawn_async(demo_all(), sched);
  sched.run();
  (void)r.get();
}
```

### Coroutine mutex

```cpp
#include <iostream>
#include "shcoro/stackless/mutex_lock.hpp"
#include "shcoro/stackless/utility.hpp"

using shcoro::Async;
using shcoro::MutexLock;
using shcoro::spawn_async;

MutexLock m;

Async<void> critical() {
  co_await m.lock();
  std::cout << "in critical section\n";
  m.unlock();
  co_return;
}

int main() {
  auto r = spawn_async(critical());
  (void)r.get();
}
```

### Generator

```cpp
#include <iostream>
#include "shcoro/stackless/generator.hpp"

shcoro::Generator<uint64_t> seq() {
  for (uint64_t i = 0; i < 5; ++i) co_yield i;
}

int main() {
  for (auto v : seq()) std::cout << v << "\n";
}
```

## Project layout

- `include/shcoro/stackless/`: public headers (`Async`, `Generator`, `MutexLock`, scheduler/timer, mux combinators)
- `demo/`: runnable examples (async demos 1–6, generator demo)
- `test/`: GTest-based tests (currently generator test)