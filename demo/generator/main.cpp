#include <iostream>

#include "shcoro/stackless/generator.hpp"

int main() {
    auto gen = []() -> shcoro::Generator<uint64_t> {
        for (auto i = 0ul; i < 100; i++) {
            co_yield i;
        }
    };

    int cnt = 0;
    // Generate the next number until its greater than count to.
    for (auto val : gen()) {
        std::cout << val << std::endl;
        cnt++;
    }
    return 0;
}