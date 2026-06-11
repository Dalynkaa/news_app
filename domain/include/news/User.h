#pragma once

#include <string>
#include "news/DateTime.h"

namespace news {

class User {
public:
    User(int id,
         std::string username,
         std::string email,
         std::string passwordHash,
         DateTime registeredAt);

    virtual ~User() = default;

    User(const User&) = delete;
    User& operator=(const User&) = delete;
    User(User&&) noexcept = default;
    User& operator=(User&&) noexcept = default;

    int id() const { return id_; }
    const std::string& username() const { return username_; }
    const std::string& email() const { return email_; }
    const std::string& passwordHash() const { return passwordHash_; }
    const DateTime& registeredAt() const { return registeredAt_; }

    void setUsername(std::string v) { username_ = std::move(v); }
    void setEmail(std::string v) { email_ = std::move(v); }
    void setPasswordHash(std::string v) { passwordHash_ = std::move(v); }

    virtual std::string role() const = 0;
    virtual std::string display() const;
    virtual bool canPublish() const { return false; }

protected:
    int id_;
    std::string username_;
    std::string email_;
    std::string passwordHash_;
    DateTime registeredAt_;
};

} // namespace news
