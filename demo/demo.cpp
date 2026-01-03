#include <iostream>

#include "shcoro/stackless/generator.hpp"

int main() {
    auto gen = []() -> shcoro::Generator<uint64_t> {
        uint64_t i = 0;
        while (true) {
            co_yield i;
            ++i;
        }
    };

    int cnt = 0;
    // Generate the next number until its greater than count to.
    for (auto val : gen()) {
        std::cout << val << std::endl;
        cnt++;
        if (val >= 100) {
            break;
        }
    }
    return 0;
}