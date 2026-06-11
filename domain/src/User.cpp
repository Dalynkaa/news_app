#include "news/User.h"

namespace news {

User::User(int id,
           std::string username,
           std::string email,
           std::string passwordHash,
           DateTime registeredAt)
    : id_(id),
      username_(std::move(username)),
      email_(std::move(email)),
      passwordHash_(std::move(passwordHash)),
      registeredAt_(registeredAt) {}

std::string User::display() const {
    return username_ + " <" + email_ + ">";
}

} // namespace news
