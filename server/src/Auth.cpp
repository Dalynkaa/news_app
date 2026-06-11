#include "Auth.h"

#include <chrono>
#include <functional>
#include <iomanip>
#include <random>
#include <sstream>

namespace news_server {

namespace {

std::string randomHex(int n) {
    static thread_local std::mt19937_64 rng{
        static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count())};
    std::uniform_int_distribution<int> d(0, 15);
    static const char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(n);
    for (int i = 0; i < n; ++i) out.push_back(hex[d(rng)]);
    return out;
}

std::string toHex(unsigned long long v) {
    std::ostringstream os;
    os << std::hex << std::setw(16) << std::setfill('0') << v;
    return os.str();
}

} // namespace

std::string hashPassword(const std::string& password) {
    std::string salt = randomHex(16);
    auto h = std::hash<std::string>{}(salt + password);
    return salt + "$" + toHex(static_cast<unsigned long long>(h));
}

bool verifyPassword(const std::string& password, const std::string& stored) {
    auto pos = stored.find('$');
    if (pos == std::string::npos) return false;
    std::string salt = stored.substr(0, pos);
    std::string hash = stored.substr(pos + 1);
    auto h = std::hash<std::string>{}(salt + password);
    return toHex(static_cast<unsigned long long>(h)) == hash;
}

} // namespace news_server
