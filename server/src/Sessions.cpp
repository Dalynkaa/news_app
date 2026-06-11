#include "Sessions.h"

#include <iomanip>
#include <random>
#include <sstream>

namespace news_server {

std::string generateToken(std::size_t bytes) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 255);
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < bytes; ++i) {
        os << std::setw(2) << dist(rng);
    }
    return os.str();
}

std::string SessionStore::create(int authorId) {
    std::lock_guard<std::mutex> lk(mutex_);
    std::string token = generateToken(24);
    sessions_[token] = {authorId, std::chrono::steady_clock::now()};
    return token;
}

void SessionStore::destroy(const std::string& token) {
    std::lock_guard<std::mutex> lk(mutex_);
    sessions_.erase(token);
}

std::optional<int> SessionStore::authorId(const std::string& token) const {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = sessions_.find(token);
    if (it == sessions_.end()) return std::nullopt;
    // Тривалість сесії — 7 діб.
    auto age = std::chrono::steady_clock::now() - it->second.createdAt;
    if (age > std::chrono::hours(24 * 7)) return std::nullopt;
    return it->second.authorId;
}

std::string extractCookie(const std::string& header, const std::string& name) {
    std::size_t pos = 0;
    while (pos < header.size()) {
        // пропустити пробіли
        while (pos < header.size() && (header[pos] == ' ' || header[pos] == ';')) ++pos;
        std::size_t eq = header.find('=', pos);
        if (eq == std::string::npos) break;
        std::string key = header.substr(pos, eq - pos);
        std::size_t semi = header.find(';', eq);
        std::string val = header.substr(eq + 1,
            semi == std::string::npos ? std::string::npos : semi - eq - 1);
        if (key == name) return val;
        pos = (semi == std::string::npos) ? header.size() : semi + 1;
    }
    return {};
}

} // namespace news_server
