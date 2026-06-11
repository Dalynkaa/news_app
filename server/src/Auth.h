#pragma once

#include <string>

namespace news_server {

// Простий хеш пароля з випадковою сіллю.
// Формат у файлі бази: "salt$hex".
// Зроблено через std::hash — для навчальної програми достатньо;
// у реальному застосунку потрібен bcrypt/argon2.
std::string hashPassword(const std::string& password);
bool        verifyPassword(const std::string& password, const std::string& stored);

} // namespace news_server
