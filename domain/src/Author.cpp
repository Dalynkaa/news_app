#include "news/Author.h"

namespace news {

Author::Author(int id,
               std::string username,
               std::string email,
               std::string passwordHash,
               DateTime registeredAt,
               std::string bio)
    : User(id, std::move(username), std::move(email),
           std::move(passwordHash), registeredAt),
      bio_(std::move(bio)) {}

std::string Author::display() const {
    std::string base = User::display();
    if (!bio_.empty()) {
        base += " — " + bio_;
    }
    return base;
}

} // namespace news
