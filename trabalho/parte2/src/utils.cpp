#include "utils.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>

std::string timestamp_now() {
    using namespace std::chrono;
    auto now = system_clock::now();
    time_t t = system_clock::to_time_t(now);
    tm tm = *localtime(&t);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}
