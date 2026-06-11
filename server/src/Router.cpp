#include "Router.h"
#include "Html.h"
#include "Sessions.h"
#include "Auth.h"

#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace news_server {

using news::Author;
using news::Category;
using news::DateTime;
using news::News;
using news::NewsRepository;
using news::SearchCriteria;
using news::SortBy;
using news::SortDirection;
using news::Tag;

namespace {

// ----------------------------------------------------------------------
// Допоміжні
// ----------------------------------------------------------------------

// Повертає id залогіненого автора (за кукою) або nullopt.
std::optional<int> currentAuthorId(const httplib::Request& req,
                                   const SessionStore& sessions) {
    auto cookies = req.get_header_value("Cookie");
    std::string token = extractCookie(cookies, "session");
    if (token.empty()) return std::nullopt;
    return sessions.authorId(token);
}

std::string currentUsername(const httplib::Request& req,
                            const SessionStore& sessions,
                            const NewsRepository& repo) {
    auto id = currentAuthorId(req, sessions);
    if (!id) return {};
    if (const Author* a = repo.findAuthor(*id)) return a->username();
    return {};
}

// Перетворює список tag-id у CSV рядок назв (з посиланнями).
std::string renderTagBadges(const std::vector<int>& tagIds,
                            const NewsRepository& repo) {
    std::ostringstream os;
    for (int id : tagIds) {
        if (const Tag* t = repo.findTag(id)) {
            os << "<a href=\"/tag/" << t->slug() << "\">#"
               << escapeHtml(t->name()) << "</a> ";
        }
    }
    return os.str();
}

// Картка однієї новини у списку.
std::string renderNewsCard(const News& n, const NewsRepository& repo) {
    std::ostringstream os;
    const Author* a = repo.findAuthor(n.authorId());
    const Category* c = repo.findCategory(n.categoryId());

    os << "<div class=\"card\">"
       << "<h2><a href=\"/news/" << n.id() << "\">" << escapeHtml(n.title()) << "</a></h2>"
       << "<div class=\"meta\">"
       << escapeHtml(n.publishedAt().toHuman());
    if (a) os << " · автор <a href=\"/author/" << escapeHtml(a->username())
              << "\">" << escapeHtml(a->username()) << "</a>";
    if (c) os << " · рубрика <a href=\"/category/" << c->slug()
              << "\">" << escapeHtml(c->name()) << "</a>";
    os << "</div>"
       << "<p>" << escapeHtml(n.excerpt(280)) << "</p>"
       << "<div class=\"tags\">" << renderTagBadges(n.tagIds(), repo) << "</div>"
       << "</div>";
    return os.str();
}

std::string renderFilterBar(const NewsRepository& repo,
                            const SearchCriteria& sc,
                            SortBy sortBy) {
    std::ostringstream os;
    os << "<form class=\"filter-bar\" method=\"get\" action=\"/\"><div class=\"row\">"
       << "<label>Назва<input name=\"q\" value=\""
       << escapeHtml(sc.titleContains.value_or("")) << "\"/></label>"
       << "<label>Рубрика<select name=\"category\"><option value=\"\">— будь-яка —</option>";
    for (const auto& c : repo.allCategories()) {
        os << "<option value=\"" << c.id() << "\""
           << (sc.categoryId && *sc.categoryId == c.id() ? " selected" : "")
           << ">" << escapeHtml(c.name()) << "</option>";
    }
    os << "</select></label>"
       << "<label>Тег<select name=\"tag\"><option value=\"\">— будь-який —</option>";
    for (const auto& t : repo.allTags()) {
        os << "<option value=\"" << t.id() << "\""
           << (sc.tagId && *sc.tagId == t.id() ? " selected" : "")
           << ">" << escapeHtml(t.name()) << "</option>";
    }
    os << "</select></label>"
       << "</div><div class=\"row\">"
       << "<label>Від<input type=\"date\" name=\"from\" value=\""
       << (sc.from ? sc.from->toDate() : "") << "\"/></label>"
       << "<label>До<input type=\"date\" name=\"to\" value=\""
       << (sc.to ? sc.to->toDate() : "") << "\"/></label>"
       << "<label>Сортування<select name=\"sort\">"
       << "<option value=\"date\""    << (sortBy == SortBy::Date     ? " selected" : "") << ">за датою</option>"
       << "<option value=\"title\""   << (sortBy == SortBy::Title    ? " selected" : "") << ">за назвою</option>"
       << "<option value=\"author\""  << (sortBy == SortBy::Author   ? " selected" : "") << ">за автором</option>"
       << "<option value=\"category\""<< (sortBy == SortBy::Category ? " selected" : "") << ">за рубрикою</option>"
       << "</select></label>"
       << "<label style=\"justify-content:flex-end\"><span>&nbsp;</span>"
       << "<button>Знайти</button></label>"
       << "</div></form>";
    return os.str();
}

// ----------------------------------------------------------------------
// Маршрути
// ----------------------------------------------------------------------

void homeRoute(const httplib::Request& req,
               httplib::Response& res,
               NewsRepository& repo,
               SessionStore& sessions) {
    SearchCriteria sc;
    if (req.has_param("q") && !req.get_param_value("q").empty())
        sc.titleContains = req.get_param_value("q");
    if (req.has_param("category") && !req.get_param_value("category").empty())
        sc.categoryId = std::stoi(req.get_param_value("category"));
    if (req.has_param("tag") && !req.get_param_value("tag").empty())
        sc.tagId = std::stoi(req.get_param_value("tag"));
    if (req.has_param("from") && !req.get_param_value("from").empty()) {
        try { sc.from = DateTime::parse(req.get_param_value("from")); } catch (...) {}
    }
    if (req.has_param("to") && !req.get_param_value("to").empty()) {
        try { sc.to = DateTime::parse(req.get_param_value("to") + "T23:59:59"); } catch (...) {}
    }
    SortBy sortBy = SortBy::Date;
    if (req.has_param("sort")) {
        auto s = req.get_param_value("sort");
        if (s == "title")    sortBy = SortBy::Title;
        else if (s == "author")   sortBy = SortBy::Author;
        else if (s == "category") sortBy = SortBy::Category;
    }
    auto items = repo.search(sc, sortBy, SortDirection::Desc);

    std::ostringstream body;
    body << renderFilterBar(repo, sc, sortBy);
    if (items.empty()) {
        body << "<div class=\"empty\">Новин не знайдено.</div>";
    } else {
        for (const auto& n : items) body << renderNewsCard(n, repo);
    }
    res.set_content(renderLayout("Головна", body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void singleNewsRoute(const httplib::Request& req,
                     httplib::Response& res,
                     NewsRepository& repo,
                     SessionStore& sessions) {
    int id = std::stoi(req.matches[1].str());
    auto opt = repo.findNews(id);
    if (!opt) { res.status = 404; res.set_content("Новину не знайдено", "text/plain"); return; }
    const News& n = *opt;
    const Author* a = repo.findAuthor(n.authorId());
    const Category* c = repo.findCategory(n.categoryId());
    auto curId = currentAuthorId(req, sessions);
    bool isOwner = curId && *curId == n.authorId();

    std::ostringstream body;
    body << "<article class=\"card\">"
         << "<h2>" << escapeHtml(n.title()) << "</h2>"
         << "<div class=\"meta\">"
         << escapeHtml(n.publishedAt().toHuman());
    if (a) body << " · <a href=\"/author/" << escapeHtml(a->username())
                << "\">" << escapeHtml(a->username()) << "</a>";
    if (c) body << " · <a href=\"/category/" << c->slug()
                << "\">" << escapeHtml(c->name()) << "</a>";
    body << "</div>"
         << "<p>" << nl2br(escapeHtml(n.content())) << "</p>"
         << "<div class=\"tags\">" << renderTagBadges(n.tagIds(), repo) << "</div>";
    if (isOwner) {
        body << "<div style=\"margin-top:1rem;display:flex;gap:0.5rem\">"
             << "<a class=\"btn btn-ghost\" href=\"/news/" << n.id() << "/edit\">Редагувати</a>"
             << "<form method=\"post\" action=\"/news/" << n.id() << "/delete\" "
                "onsubmit=\"return confirm('Видалити цю новину?')\">"
                "<button class=\"btn btn-danger\">Видалити</button></form></div>";
    }
    body << "</article>";

    res.set_content(renderLayout(n.title(), body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

std::string renderNewsForm(const NewsRepository& repo,
                           const std::string& action,
                           const std::string& title = {},
                           const std::string& content = {},
                           int categoryId = 0,
                           const std::string& tagsCsv = {},
                           const std::string& error = {}) {
    std::ostringstream os;
    if (!error.empty()) os << "<div class=\"alert\">" << escapeHtml(error) << "</div>";
    os << "<form method=\"post\" action=\"" << action << "\" class=\"card\">"
       << "<label>Заголовок<input name=\"title\" required value=\""
       << escapeHtml(title) << "\"/></label>"
       << "<label>Рубрика<select name=\"category\" required>"
       << "<option value=\"\">— оберіть —</option>";
    for (const auto& c : repo.allCategories()) {
        os << "<option value=\"" << c.id() << "\""
           << (categoryId == c.id() ? " selected" : "")
           << ">" << escapeHtml(c.name()) << "</option>";
    }
    os << "</select></label>"
       << "<label>Теги (через кому)<input name=\"tags\" value=\""
       << escapeHtml(tagsCsv) << "\" placeholder=\"україна, спорт, AI\"/></label>"
       << "<label>Текст новини<textarea name=\"content\" required>"
       << escapeHtml(content) << "</textarea></label>"
       << "<button>Зберегти</button></form>";
    return os.str();
}

std::vector<int> parseTagsCsv(const std::string& csv, NewsRepository& repo) {
    std::vector<int> ids;
    std::string cur;
    auto flush = [&]() {
        // trim
        while (!cur.empty() && std::isspace(static_cast<unsigned char>(cur.front()))) cur.erase(0, 1);
        while (!cur.empty() && std::isspace(static_cast<unsigned char>(cur.back())))  cur.pop_back();
        if (!cur.empty()) ids.push_back(repo.ensureTag(cur));
        cur.clear();
    };
    for (char c : csv) {
        if (c == ',') flush();
        else cur += c;
    }
    flush();
    return ids;
}

std::string tagsToCsv(const std::vector<int>& ids, const NewsRepository& repo) {
    std::string out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (const Tag* t = repo.findTag(ids[i])) {
            if (!out.empty()) out += ", ";
            out += t->name();
        }
    }
    return out;
}

void newNewsForm(const httplib::Request& req,
                 httplib::Response& res,
                 NewsRepository& repo,
                 SessionStore& sessions) {
    if (!currentAuthorId(req, sessions)) { res.set_redirect("/login"); return; }
    res.set_content(renderLayout("Нова новина",
        renderNewsForm(repo, "/news"),
        currentUsername(req, sessions, repo)), "text/html; charset=utf-8");
}

void createNews(const httplib::Request& req,
                httplib::Response& res,
                NewsRepository& repo,
                SessionStore& sessions,
                const std::string& dataFile) {
    auto authorId = currentAuthorId(req, sessions);
    if (!authorId) { res.set_redirect("/login"); return; }
    try {
        std::string title    = req.get_param_value("title");
        std::string content  = req.get_param_value("content");
        int categoryId       = std::stoi(req.get_param_value("category"));
        auto tagIds          = parseTagsCsv(req.get_param_value("tags"), repo);

        int id = repo.addNews(*authorId, title, content, categoryId, tagIds);
        repo.saveToFile(dataFile);
        res.set_redirect("/news/" + std::to_string(id));
    } catch (const std::exception& e) {
        res.set_content(renderLayout("Нова новина",
            renderNewsForm(repo, "/news",
                req.get_param_value("title"),
                req.get_param_value("content"),
                req.has_param("category") && !req.get_param_value("category").empty() ?
                    std::stoi(req.get_param_value("category")) : 0,
                req.get_param_value("tags"),
                e.what()),
            currentUsername(req, sessions, repo)), "text/html; charset=utf-8");
    }
}

void editNewsForm(const httplib::Request& req,
                  httplib::Response& res,
                  NewsRepository& repo,
                  SessionStore& sessions) {
    int id = std::stoi(req.matches[1].str());
    auto opt = repo.findNews(id);
    auto cur = currentAuthorId(req, sessions);
    if (!opt) { res.status = 404; res.set_content("Не знайдено", "text/plain"); return; }
    if (!cur || *cur != opt->authorId()) { res.status = 403; res.set_content("Заборонено", "text/plain"); return; }

    res.set_content(renderLayout("Редагування",
        renderNewsForm(repo, "/news/" + std::to_string(id),
                       opt->title(), opt->content(), opt->categoryId(),
                       tagsToCsv(opt->tagIds(), repo)),
        currentUsername(req, sessions, repo)), "text/html; charset=utf-8");
}

void updateNews(const httplib::Request& req,
                httplib::Response& res,
                NewsRepository& repo,
                SessionStore& sessions,
                const std::string& dataFile) {
    int id = std::stoi(req.matches[1].str());
    auto opt = repo.findNews(id);
    auto cur = currentAuthorId(req, sessions);
    if (!opt) { res.status = 404; return; }
    if (!cur || *cur != opt->authorId()) { res.status = 403; return; }
    try {
        auto tagIds = parseTagsCsv(req.get_param_value("tags"), repo);
        repo.replaceNews(id,
                         req.get_param_value("title"),
                         req.get_param_value("content"),
                         std::stoi(req.get_param_value("category")),
                         tagIds);
        repo.saveToFile(dataFile);
        res.set_redirect("/news/" + std::to_string(id));
    } catch (const std::exception& e) {
        res.set_content(renderLayout("Редагування",
            renderNewsForm(repo, "/news/" + std::to_string(id),
                           req.get_param_value("title"),
                           req.get_param_value("content"),
                           req.has_param("category") && !req.get_param_value("category").empty()
                               ? std::stoi(req.get_param_value("category")) : 0,
                           req.get_param_value("tags"),
                           e.what()),
            currentUsername(req, sessions, repo)), "text/html; charset=utf-8");
    }
}

void deleteNews(const httplib::Request& req,
                httplib::Response& res,
                NewsRepository& repo,
                SessionStore& sessions,
                const std::string& dataFile) {
    int id = std::stoi(req.matches[1].str());
    auto opt = repo.findNews(id);
    auto cur = currentAuthorId(req, sessions);
    if (!opt) { res.status = 404; return; }
    if (!cur || *cur != opt->authorId()) { res.status = 403; return; }
    repo.removeNews(id);
    repo.saveToFile(dataFile);
    res.set_redirect("/");
}

// ----------------------------------------------------------------------
// Авторизація
// ----------------------------------------------------------------------

std::string renderRegisterForm(const std::string& error = {}) {
    std::ostringstream os;
    if (!error.empty()) os << "<div class=\"alert\">" << escapeHtml(error) << "</div>";
    os << "<form method=\"post\" action=\"/register\" class=\"card\">"
       << "<h2>Реєстрація автора</h2>"
       << "<label>Ім'я користувача<input name=\"username\" required minlength=\"3\"/></label>"
       << "<label>Електронна адреса<input name=\"email\" type=\"email\" required/></label>"
       << "<label>Пароль<input name=\"password\" type=\"password\" required minlength=\"6\"/></label>"
       << "<label>Біографія (необов'язково)<textarea name=\"bio\"></textarea></label>"
       << "<button>Зареєструватися</button>"
       << "<p style=\"text-align:center;color:var(--muted)\">Вже маєте акаунт? "
       << "<a href=\"/login\">Увійти</a></p>"
       << "</form>";
    return os.str();
}

std::string renderLoginForm(const std::string& error = {}) {
    std::ostringstream os;
    if (!error.empty()) os << "<div class=\"alert\">" << escapeHtml(error) << "</div>";
    os << "<form method=\"post\" action=\"/login\" class=\"card\">"
       << "<h2>Вхід</h2>"
       << "<label>Ім'я користувача<input name=\"username\" required/></label>"
       << "<label>Пароль<input name=\"password\" type=\"password\" required/></label>"
       << "<button>Увійти</button>"
       << "<p style=\"text-align:center;color:var(--muted)\">Ще нема акаунта? "
       << "<a href=\"/register\">Зареєструватися</a></p>"
       << "</form>";
    return os.str();
}

void registerForm(const httplib::Request& req,
                  httplib::Response& res,
                  NewsRepository& repo,
                  SessionStore& sessions) {
    res.set_content(renderLayout("Реєстрація", renderRegisterForm(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void doRegister(const httplib::Request& req,
                httplib::Response& res,
                NewsRepository& repo,
                SessionStore& sessions,
                const std::string& dataFile) {
    try {
        std::string username = req.get_param_value("username");
        std::string email    = req.get_param_value("email");
        std::string password = req.get_param_value("password");
        std::string bio      = req.get_param_value("bio");
        if (password.size() < 6) throw std::invalid_argument("Пароль занадто короткий");
        int id = repo.registerAuthor(username, email, hashPassword(password), bio);
        repo.saveToFile(dataFile);

        std::string token = sessions.create(id);
        res.set_header("Set-Cookie", "session=" + token + "; Path=/; HttpOnly; SameSite=Lax");
        res.set_redirect("/");
    } catch (const std::exception& e) {
        res.set_content(renderLayout("Реєстрація", renderRegisterForm(e.what()),
                                     currentUsername(req, sessions, repo)),
                        "text/html; charset=utf-8");
    }
}

void loginForm(const httplib::Request& req,
               httplib::Response& res,
               NewsRepository& repo,
               SessionStore& sessions) {
    res.set_content(renderLayout("Вхід", renderLoginForm(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void doLogin(const httplib::Request& req,
             httplib::Response& res,
             NewsRepository& repo,
             SessionStore& sessions) {
    std::string username = req.get_param_value("username");
    std::string password = req.get_param_value("password");
    const Author* a = repo.findAuthorByUsername(username);
    if (!a || !verifyPassword(password, a->passwordHash())) {
        res.set_content(renderLayout("Вхід",
            renderLoginForm("Невірне ім'я користувача або пароль"),
            currentUsername(req, sessions, repo)), "text/html; charset=utf-8");
        return;
    }
    std::string token = sessions.create(a->id());
    res.set_header("Set-Cookie", "session=" + token + "; Path=/; HttpOnly; SameSite=Lax");
    res.set_redirect("/");
}

void doLogout(const httplib::Request& req, httplib::Response& res, SessionStore& sessions) {
    auto cookies = req.get_header_value("Cookie");
    std::string token = extractCookie(cookies, "session");
    if (!token.empty()) sessions.destroy(token);
    res.set_header("Set-Cookie", "session=; Path=/; Max-Age=0");
    res.set_redirect("/");
}

// ----------------------------------------------------------------------
// Списки за рубрикою / тегом / автором + індексні сторінки
// ----------------------------------------------------------------------

void categoryRoute(const httplib::Request& req,
                   httplib::Response& res,
                   NewsRepository& repo,
                   SessionStore& sessions) {
    std::string slug = req.matches[1].str();
    const Category* c = repo.findCategoryBySlug(slug);
    if (!c) { res.status = 404; res.set_content("Рубрику не знайдено", "text/plain"); return; }

    SearchCriteria sc; sc.categoryId = c->id();
    auto items = repo.search(sc, SortBy::Date, SortDirection::Desc);

    std::ostringstream body;
    body << "<h2 style=\"margin:0 0 1rem 0\">Рубрика: " << escapeHtml(c->name()) << "</h2>";
    if (!c->description().empty())
        body << "<p style=\"color:var(--muted);margin-bottom:1.5rem\">" << escapeHtml(c->description()) << "</p>";
    if (items.empty()) body << "<div class=\"empty\">У цій рубриці поки нема новин.</div>";
    else for (const auto& n : items) body << renderNewsCard(n, repo);

    res.set_content(renderLayout(c->name(), body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void tagRoute(const httplib::Request& req,
              httplib::Response& res,
              NewsRepository& repo,
              SessionStore& sessions) {
    std::string slug = req.matches[1].str();
    const Tag* t = repo.findTagBySlug(slug);
    if (!t) { res.status = 404; res.set_content("Тег не знайдено", "text/plain"); return; }

    SearchCriteria sc; sc.tagId = t->id();
    auto items = repo.search(sc, SortBy::Date, SortDirection::Desc);

    std::ostringstream body;
    body << "<h2 style=\"margin:0 0 1rem 0\">Тег: #" << escapeHtml(t->name()) << "</h2>";
    if (items.empty()) body << "<div class=\"empty\">З цим тегом нема новин.</div>";
    else for (const auto& n : items) body << renderNewsCard(n, repo);

    res.set_content(renderLayout("Тег: " + t->name(), body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void authorRoute(const httplib::Request& req,
                 httplib::Response& res,
                 NewsRepository& repo,
                 SessionStore& sessions) {
    std::string uname = req.matches[1].str();
    const Author* a = repo.findAuthorByUsername(uname);
    if (!a) { res.status = 404; res.set_content("Автора не знайдено", "text/plain"); return; }

    SearchCriteria sc; sc.authorId = a->id();
    auto items = repo.search(sc, SortBy::Date, SortDirection::Desc);

    std::ostringstream body;
    body << "<div class=\"card\"><h2>" << escapeHtml(a->username()) << "</h2>"
         << "<div class=\"meta\">" << escapeHtml(a->email())
         << " · з нами з " << escapeHtml(a->registeredAt().toDate()) << "</div>";
    if (!a->bio().empty()) body << "<p>" << escapeHtml(a->bio()) << "</p>";
    body << "</div><h3 style=\"margin-top:1.5rem\">Публікації</h3>";
    if (items.empty()) body << "<div class=\"empty\">Автор поки нічого не опублікував.</div>";
    else for (const auto& n : items) body << renderNewsCard(n, repo);

    res.set_content(renderLayout(a->username(), body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void categoryIndex(const httplib::Request& req,
                   httplib::Response& res,
                   NewsRepository& repo,
                   SessionStore& sessions) {
    std::ostringstream body;
    body << "<h2 style=\"margin:0 0 1rem 0\">Всі рубрики</h2>";
    auto cats = repo.allCategories();
    if (cats.empty()) body << "<div class=\"empty\">Рубрик ще нема.</div>";
    for (const auto& c : cats) {
        body << "<div class=\"card\">"
             << "<h2><a href=\"/category/" << c.slug() << "\">" << escapeHtml(c.name()) << "</a></h2>";
        if (!c.description().empty())
            body << "<p style=\"color:var(--muted);margin:0\">" << escapeHtml(c.description()) << "</p>";
        body << "</div>";
    }
    res.set_content(renderLayout("Рубрики", body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void tagIndex(const httplib::Request& req,
              httplib::Response& res,
              NewsRepository& repo,
              SessionStore& sessions) {
    std::ostringstream body;
    body << "<h2 style=\"margin:0 0 1rem 0\">Всі теги</h2><div class=\"card\">";
    auto tags = repo.allTags();
    if (tags.empty()) body << "<div class=\"empty\">Тегів ще нема.</div>";
    for (const auto& t : tags) {
        body << "<a class=\"btn btn-ghost\" style=\"margin:0.25rem\" href=\"/tag/"
             << t.slug() << "\">#" << escapeHtml(t.name()) << "</a>";
    }
    body << "</div>";
    res.set_content(renderLayout("Теги", body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

void authorIndex(const httplib::Request& req,
                 httplib::Response& res,
                 NewsRepository& repo,
                 SessionStore& sessions) {
    std::ostringstream body;
    body << "<h2 style=\"margin:0 0 1rem 0\">Зареєстровані автори</h2>";
    auto authors = repo.allAuthors();
    if (authors.empty()) body << "<div class=\"empty\">Поки нема зареєстрованих авторів.</div>";
    for (const auto* a : authors) {
        body << "<div class=\"card\">"
             << "<h2><a href=\"/author/" << escapeHtml(a->username()) << "\">"
             << escapeHtml(a->username()) << "</a></h2>"
             << "<div class=\"meta\">" << escapeHtml(a->email()) << "</div>";
        if (!a->bio().empty()) body << "<p>" << escapeHtml(a->bio()) << "</p>";
        body << "</div>";
    }
    res.set_content(renderLayout("Автори", body.str(),
                                 currentUsername(req, sessions, repo)),
                    "text/html; charset=utf-8");
}

} // namespace

// ----------------------------------------------------------------------
// Реєстрація маршрутів
// ----------------------------------------------------------------------

void registerRoutes(httplib::Server& server,
                    NewsRepository& repo,
                    SessionStore& sessions,
                    const std::string& dataFile) {
    server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << "[" << res.status << "] " << req.method << ' ' << req.path << '\n';
    });

    server.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
        homeRoute(req, res, repo, sessions);
    });

    server.Get(R"(/news/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        singleNewsRoute(req, res, repo, sessions);
    });
    server.Get("/news/new", [&](const httplib::Request& req, httplib::Response& res) {
        newNewsForm(req, res, repo, sessions);
    });
    server.Post("/news", [&](const httplib::Request& req, httplib::Response& res) {
        createNews(req, res, repo, sessions, dataFile);
    });
    server.Get(R"(/news/(\d+)/edit)", [&](const httplib::Request& req, httplib::Response& res) {
        editNewsForm(req, res, repo, sessions);
    });
    server.Post(R"(/news/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        updateNews(req, res, repo, sessions, dataFile);
    });
    server.Post(R"(/news/(\d+)/delete)", [&](const httplib::Request& req, httplib::Response& res) {
        deleteNews(req, res, repo, sessions, dataFile);
    });

    server.Get("/categories", [&](const httplib::Request& req, httplib::Response& res) {
        categoryIndex(req, res, repo, sessions);
    });
    server.Get("/tags", [&](const httplib::Request& req, httplib::Response& res) {
        tagIndex(req, res, repo, sessions);
    });
    server.Get("/authors", [&](const httplib::Request& req, httplib::Response& res) {
        authorIndex(req, res, repo, sessions);
    });

    server.Get(R"(/category/([\w\-]+))", [&](const httplib::Request& req, httplib::Response& res) {
        categoryRoute(req, res, repo, sessions);
    });
    server.Get(R"(/tag/([\w\-]+))", [&](const httplib::Request& req, httplib::Response& res) {
        tagRoute(req, res, repo, sessions);
    });
    server.Get(R"(/author/([\w\-]+))", [&](const httplib::Request& req, httplib::Response& res) {
        authorRoute(req, res, repo, sessions);
    });

    server.Get("/register", [&](const httplib::Request& req, httplib::Response& res) {
        registerForm(req, res, repo, sessions);
    });
    server.Post("/register", [&](const httplib::Request& req, httplib::Response& res) {
        doRegister(req, res, repo, sessions, dataFile);
    });
    server.Get("/login", [&](const httplib::Request& req, httplib::Response& res) {
        loginForm(req, res, repo, sessions);
    });
    server.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
        doLogin(req, res, repo, sessions);
    });
    server.Post("/logout", [&](const httplib::Request& req, httplib::Response& res) {
        doLogout(req, res, sessions);
    });
}

} // namespace news_server
