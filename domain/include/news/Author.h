#pragma once

#include "news/User.h"

namespace news {

class Author : public User {
public:
    Author(int id,
           std::string username,
           std::string email,
           std::string passwordHash,
           DateTime registeredAt,
           std::string bio = {});

    const std::string& bio() const { return bio_; }
    void setBio(std::string v) { bio_ = std::move(v); }

    std::string role() const override { return "author"; }
    bool canPublish() const override { return true; }
    std::string display() const override;

private:
    std::string bio_;
};

} // namespace news
