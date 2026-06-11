#pragma once

#include <string>

namespace news_server {

std::string escapeHtml(const std::string& s);
std::string nl2br(const std::string& escaped);
std::string renderLayout(const std::string& title,
                         const std::string& body,
                         const std::string& currentUser);

} // namespace news_server
