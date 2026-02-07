#pragma once

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class Logger {
   private:
    template <typename... Args>
    static std::string formatArgs(Args... args) {
        std::ostringstream oss;
        (oss << ... << args);
        return oss.str();
    }

   public:
    template <typename... Args>
    static void log(const char* file, const char* function, int line, Args... args) {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);

        std::cout << "[" << file << ":" << function << ":" << line << "] "
                  << formatArgs(args...) << std::endl;
    }
};

#ifdef SHCORO_ENABLE_LOG
#define SHCORO_LOG(...) Logger::log(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define SHCORO_LOG(...) \
    do {                \
    } while (0)
#endif
