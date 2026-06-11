// Курсова робота, варіант 10: сайт онлайн-новин.

#include <httplib.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

#include "news/NewsRepository.h"
#include "Router.h"
#include "Sessions.h"
#include "Auth.h"

namespace {

std::atomic<httplib::Server*> g_server{nullptr};

void onSignal(int) {
    if (auto* s = g_server.load()) s->stop();
}

// Якщо база порожня, кладемо туди початковий контент.
void seedIfEmpty(news::NewsRepository& repo) {
    using news::DateTime;
    if (repo.newsCount() > 0) return;

    int catSport  = repo.addCategory("Спорт",       "Найважливіші події українського та світового спорту");
    int catTech   = repo.addCategory("Технології",  "Новини IT, штучного інтелекту та гаджетів");
    int catWorld  = repo.addCategory("Світ",        "Міжнародні події та політика");

    int tagUa = repo.addTag("Україна");
    int tagAi = repo.addTag("AI");
    int tagFb = repo.addTag("Футбол");
    int tagCp = repo.addTag("Кіберспорт");

    int demoId = repo.registerAuthor(
        "demo", "demo@news.local",
        news_server::hashPassword("demo123"),
        "Демо-автор для першого запуску. Пароль: demo123");

    repo.addNews(demoId,
        "Збірна України здобула важливу перемогу",
        "У контрольному матчі національна збірна перемогла з рахунком 2:1.\n"
        "Голи забили нападники першого ескадрону. Тренерський штаб задоволений грою.",
        catSport, {tagUa, tagFb},
        DateTime(2026, 5, 14, 19, 30));

    repo.addNews(demoId,
        "Новий рівень моделей штучного інтелекту",
        "Дослідники представили модель, що демонструє покращені результати на стандартних бенчмарках.\n"
        "Особливо помітне зростання у задачах міркування та програмування.",
        catTech, {tagAi},
        DateTime(2026, 5, 13, 11, 0));

    repo.addNews(demoId,
        "Українські кіберспортсмени у фіналі",
        "Команда вийшла до фіналу великого міжнародного турніру.\n"
        "Загалом їх чекає сильний суперник, але форма гравців обнадіює.",
        catSport, {tagUa, tagCp},
        DateTime(2026, 5, 10, 14, 15));

    repo.addNews(demoId,
        "Дипломатичні переговори завершилися рамковою угодою",
        "Сторони повідомили про підписання документа, що окреслює напрями\n"
        "співпраці. Деталі планується розкрити після ратифікації.",
        catWorld, {tagUa},
        DateTime(2026, 5, 9, 9, 45));
}

} // namespace

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    std::string dataFile = "data/news.db";

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--host" && i + 1 < argc) host = argv[++i];
        else if (a == "--port" && i + 1 < argc) port = std::stoi(argv[++i]);
        else if (a == "--data" && i + 1 < argc) dataFile = argv[++i];
        else if (a == "-h" || a == "--help") {
            std::cout << "Використання: news_server [--host HOST] [--port PORT] [--data FILE]\n";
            return 0;
        }
    }

    news::NewsRepository repo;
    if (!repo.loadFromFile(dataFile)) {
        std::cerr << "Сховище не знайдено — буде створено: " << dataFile << '\n';
    }
    seedIfEmpty(repo);
    repo.saveToFile(dataFile);

    news_server::SessionStore sessions;

    httplib::Server server;
    g_server.store(&server);
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    server.set_default_headers({{"Cache-Control", "no-cache"}});

    news_server::registerRoutes(server, repo, sessions, dataFile);

    std::cout << "============================================================\n"
              << "  Онлайн-новини — курсова з ОП (варіант 10)\n"
              << "  Сервер слухає на http://" << host << ":" << port << "\n"
              << "  Демо-логін: demo / demo123\n"
              << "  Файл даних: " << dataFile << "\n"
              << "  Для зупинки — Ctrl+C\n"
              << "============================================================\n";

    if (!server.listen(host, port)) {
        std::cerr << "Не вдалося запустити сервер на " << host << ":" << port << '\n';
        return 1;
    }
    return 0;
}
