#pragma once

#include <httplib.h>
#include "news/NewsRepository.h"
#include "Sessions.h"

namespace news_server {

// Реєструє маршрути сервера.
void registerRoutes(httplib::Server& server,
                    news::NewsRepository& repo,
                    SessionStore& sessions,
                    const std::string& dataFile);

} // namespace news_server
