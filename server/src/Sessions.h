#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace news_server {

// Сховище сесій у пам'яті: токен -> id автора.
class SessionStore {
public:
    std::string create(int authorId);
    void destroy(const std::string& token);
    std::optional<int> authorId(const std::string& token) const;

private:
    struct Entry {
        int authorId;
        std::chrono::steady_clock::time_point createdAt;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Entry> sessions_;
};

std::string extractCookie(const std::string& cookieHeader, const std::string& name);
std::string generateToken(std::size_t bytes = 24);

} // namespace news_server
