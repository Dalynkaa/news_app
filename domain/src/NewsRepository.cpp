#include "news/NewsRepository.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace news {

namespace {

// Формат файлу:
//   # news-repo v1
//   @NEXT <author> <category> <tag> <news>
//   C\t<id>\t<name>\t<description>
//   T\t<id>\t<name>
//   A\t<id>\t<username>\t<email>\t<passwordHash>\t<registered>\t<bio>
//   N\t<id>\t<authorId>\t<categoryId>\t<tagIds csv>\t<published>\t<title>\t<content>

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c;
        }
    }
    return out;
}

std::string unescape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char nx = s[i + 1];
            switch (nx) {
                case '\\': out += '\\'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                default:   out += nx;
            }
            ++i;
        } else {
            out += s[i];
        }
    }
    return out;
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

std::string joinIds(const std::vector<int>& ids) {
    std::string out;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i) out += ',';
        out += std::to_string(ids[i]);
    }
    return out;
}

std::vector<int> parseIds(const std::string& s) {
    std::vector<int> out;
    if (s.empty()) return out;
    std::string cur;
    for (char c : s) {
        if (c == ',') {
            if (!cur.empty()) out.push_back(std::stoi(cur));
            cur.clear();
        } else cur += c;
    }
    if (!cur.empty()) out.push_back(std::stoi(cur));
    return out;
}

} // namespace


NewsRepository::NewsRepository() = default;

const Author* NewsRepository::findAuthorUnlocked(int id) const {
    for (const auto& a : authors_) if (a->id() == id) return a.get();
    return nullptr;
}

const Category* NewsRepository::findCategoryUnlocked(int id) const {
    for (const auto& c : categories_) if (c.id() == id) return &c;
    return nullptr;
}

const Tag* NewsRepository::findTagUnlocked(int id) const {
    for (const auto& t : tags_) if (t.id() == id) return &t;
    return nullptr;
}

News* NewsRepository::findNewsUnlocked(int id) {
    for (auto& n : news_) if (n.id() == id) return &n;
    return nullptr;
}

const News* NewsRepository::findNewsUnlocked(int id) const {
    for (const auto& n : news_) if (n.id() == id) return &n;
    return nullptr;
}

// ---- Автори -------------------------------------------------------------

int NewsRepository::registerAuthor(const std::string& username,
                                   const std::string& email,
                                   const std::string& passwordHash,
                                   const std::string& bio) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& a : authors_) {
        if (a->username() == username)
            throw std::invalid_argument("Автор з таким логіном уже існує");
        if (a->email() == email)
            throw std::invalid_argument("Автор з такою адресою вже існує");
    }
    int id = nextAuthorId_++;
    authors_.push_back(std::make_unique<Author>(
        id, username, email, passwordHash, DateTime::now(), bio));
    return id;
}

bool NewsRepository::removeAuthor(int id) {
    std::lock_guard<std::mutex> lk(mutex_);
    // Заборонимо видалення, якщо в автора є новини (підтримуємо посилкову цілісність).
    for (const auto& n : news_) {
        if (n.authorId() == id) return false;
    }
    auto it = std::remove_if(authors_.begin(), authors_.end(),
        [&](const std::unique_ptr<Author>& a) { return a->id() == id; });
    if (it == authors_.end()) return false;
    authors_.erase(it, authors_.end());
    return true;
}

bool NewsRepository::updateAuthor(int id, const std::string& email, const std::string& bio) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& a : authors_) {
        if (a->id() == id) {
            a->setEmail(email);
            a->setBio(bio);
            return true;
        }
    }
    return false;
}

const Author* NewsRepository::findAuthor(int id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return findAuthorUnlocked(id);
}

const Author* NewsRepository::findAuthorByUsername(const std::string& username) const {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& a : authors_) if (a->username() == username) return a.get();
    return nullptr;
}

std::vector<const Author*> NewsRepository::allAuthors() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<const Author*> out;
    out.reserve(authors_.size());
    for (const auto& a : authors_) out.push_back(a.get());
    return out;
}

// ---- Рубрики ------------------------------------------------------------

int NewsRepository::addCategory(const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& c : categories_) {
        if (c.name() == name)
            throw std::invalid_argument("Рубрика з такою назвою вже існує");
    }
    int id = nextCategoryId_++;
    categories_.emplace_back(id, name, description);
    return id;
}

bool NewsRepository::removeCategory(int id) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& n : news_) {
        if (n.categoryId() == id) return false;  // referential integrity
    }
    auto it = std::remove_if(categories_.begin(), categories_.end(),
        [&](const Category& c) { return c.id() == id; });
    if (it == categories_.end()) return false;
    categories_.erase(it, categories_.end());
    return true;
}

bool NewsRepository::updateCategory(int id, const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (auto& c : categories_) {
        if (c.id() == id) {
            c.setName(name);
            c.setDescription(description);
            return true;
        }
    }
    return false;
}

const Category* NewsRepository::findCategory(int id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return findCategoryUnlocked(id);
}

const Category* NewsRepository::findCategoryBySlug(const std::string& slug) const {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& c : categories_) if (c.slug() == slug) return &c;
    return nullptr;
}

std::vector<Category> NewsRepository::allCategories() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return categories_;
}

// ---- Теги ---------------------------------------------------------------

int NewsRepository::addTag(const std::string& name) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& t : tags_) {
        if (t.name() == name) return t.id();
    }
    int id = nextTagId_++;
    tags_.emplace_back(id, name);
    return id;
}

bool NewsRepository::removeTag(int id) {
    std::lock_guard<std::mutex> lk(mutex_);
    // Видалити цей tag з усіх новин (м'яка цілісність).
    for (auto& n : news_) {
        auto t = n.tagIds();
        t.erase(std::remove(t.begin(), t.end(), id), t.end());
        n.setTagIds(std::move(t));
    }
    auto it = std::remove_if(tags_.begin(), tags_.end(),
        [&](const Tag& t) { return t.id() == id; });
    if (it == tags_.end()) return false;
    tags_.erase(it, tags_.end());
    return true;
}

const Tag* NewsRepository::findTag(int id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    return findTagUnlocked(id);
}

const Tag* NewsRepository::findTagBySlug(const std::string& slug) const {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& t : tags_) if (t.slug() == slug) return &t;
    return nullptr;
}

int NewsRepository::ensureTag(const std::string& name) {
    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& t : tags_) if (t.name() == name) return t.id();
    int id = nextTagId_++;
    tags_.emplace_back(id, name);
    return id;
}

std::vector<Tag> NewsRepository::allTags() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return tags_;
}

// ---- Новини -------------------------------------------------------------

int NewsRepository::addNews(int authorId,
                            const std::string& title,
                            const std::string& content,
                            int categoryId,
                            const std::vector<int>& tagIds,
                            DateTime publishedAt) {
    std::lock_guard<std::mutex> lk(mutex_);

    const Author* a = findAuthorUnlocked(authorId);
    if (!a) throw std::invalid_argument("Автор не існує");
    if (!a->canPublish()) throw std::invalid_argument("Користувач не має прав публікації");
    if (!findCategoryUnlocked(categoryId)) throw std::invalid_argument("Рубрика не існує");
    for (int t : tagIds) {
        if (!findTagUnlocked(t)) throw std::invalid_argument("Тег не існує: id=" + std::to_string(t));
    }
    if (title.empty()) throw std::invalid_argument("Назва новини не може бути порожньою");

    int id = nextNewsId_++;
    news_.emplace_back(id, title, content, authorId, categoryId, tagIds, publishedAt);
    return id;
}

bool NewsRepository::removeNews(int id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = std::remove_if(news_.begin(), news_.end(),
        [&](const News& n) { return n.id() == id; });
    if (it == news_.end()) return false;
    news_.erase(it, news_.end());
    return true;
}

bool NewsRepository::replaceNews(int id,
                                 const std::string& title,
                                 const std::string& content,
                                 int categoryId,
                                 const std::vector<int>& tagIds) {
    std::lock_guard<std::mutex> lk(mutex_);
    News* n = findNewsUnlocked(id);
    if (!n) return false;
    if (!findCategoryUnlocked(categoryId)) throw std::invalid_argument("Рубрика не існує");
    for (int t : tagIds) {
        if (!findTagUnlocked(t)) throw std::invalid_argument("Тег не існує: id=" + std::to_string(t));
    }
    n->setTitle(title);
    n->setContent(content);
    n->setCategoryId(categoryId);
    n->setTagIds(tagIds);
    return true;
}

std::optional<News> NewsRepository::findNews(int id) const {
    std::lock_guard<std::mutex> lk(mutex_);
    const News* n = findNewsUnlocked(id);
    if (!n) return std::nullopt;
    return *n;
}

std::vector<News> NewsRepository::allNews() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return news_;
}

std::vector<News> NewsRepository::search(const SearchCriteria& criteria,
                                         SortBy sortBy,
                                         SortDirection direction) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<News> result;
    result.reserve(news_.size());
    for (const auto& n : news_) {
        if (criteria.matches(n)) result.push_back(n);
    }

    // Підготуємо допоміжні мапи для сортування за іменами автора/категорії.
    std::unordered_map<int, std::string> authorNames, categoryNames;
    if (sortBy == SortBy::Author || sortBy == SortBy::Category) {
        for (const auto& a : authors_) authorNames[a->id()] = a->username();
        for (const auto& c : categories_) categoryNames[c.id()] = c.name();
    }

    auto cmp = [&](const News& a, const News& b) {
        bool less;
        switch (sortBy) {
            case SortBy::Date:     less = a.publishedAt() < b.publishedAt(); break;
            case SortBy::Title:    less = a.title()       < b.title();       break;
            case SortBy::Category: less = categoryNames[a.categoryId()] < categoryNames[b.categoryId()]; break;
            case SortBy::Author:   less = authorNames[a.authorId()]     < authorNames[b.authorId()];     break;
        }
        return direction == SortDirection::Asc ? less : !less;
    };
    std::sort(result.begin(), result.end(), cmp);
    return result;
}

// ---- Персистентність ----------------------------------------------------

bool NewsRepository::saveToFile(const std::string& path) const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::ofstream out(path);
    if (!out) return false;

    out << "# news-repo v1\n";
    out << "@NEXT " << nextAuthorId_ << ' ' << nextCategoryId_ << ' '
        << nextTagId_ << ' ' << nextNewsId_ << '\n';

    for (const auto& c : categories_) {
        out << "C\t" << c.id() << '\t' << escape(c.name()) << '\t'
            << escape(c.description()) << '\n';
    }
    for (const auto& t : tags_) {
        out << "T\t" << t.id() << '\t' << escape(t.name()) << '\n';
    }
    for (const auto& a : authors_) {
        out << "A\t" << a->id() << '\t' << escape(a->username()) << '\t'
            << escape(a->email()) << '\t' << escape(a->passwordHash()) << '\t'
            << a->registeredAt().toUnix() << '\t' << escape(a->bio()) << '\n';
    }
    for (const auto& n : news_) {
        out << "N\t" << n.id() << '\t' << n.authorId() << '\t' << n.categoryId() << '\t'
            << joinIds(n.tagIds()) << '\t' << n.publishedAt().toUnix() << '\t'
            << escape(n.title()) << '\t' << escape(n.content()) << '\n';
    }
    return out.good();
}

bool NewsRepository::loadFromFile(const std::string& path) {
    std::lock_guard<std::mutex> lk(mutex_);
    std::ifstream in(path);
    if (!in) return false;

    authors_.clear();
    categories_.clear();
    tags_.clear();
    news_.clear();
    nextAuthorId_ = nextCategoryId_ = nextTagId_ = nextNewsId_ = 1;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("@NEXT ", 0) == 0) {
            std::istringstream ss(line.substr(6));
            ss >> nextAuthorId_ >> nextCategoryId_ >> nextTagId_ >> nextNewsId_;
            continue;
        }
        auto parts = split(line, '\t');
        if (parts.empty()) continue;

        try {
            if (parts[0] == "C" && parts.size() >= 4) {
                categories_.emplace_back(std::stoi(parts[1]),
                                         unescape(parts[2]),
                                         unescape(parts[3]));
            } else if (parts[0] == "T" && parts.size() >= 3) {
                tags_.emplace_back(std::stoi(parts[1]), unescape(parts[2]));
            } else if (parts[0] == "A" && parts.size() >= 7) {
                authors_.push_back(std::make_unique<Author>(
                    std::stoi(parts[1]),
                    unescape(parts[2]),
                    unescape(parts[3]),
                    unescape(parts[4]),
                    DateTime::fromUnix(std::stoll(parts[5])),
                    unescape(parts[6])));
            } else if (parts[0] == "N" && parts.size() >= 8) {
                news_.emplace_back(std::stoi(parts[1]),
                                   unescape(parts[6]),
                                   unescape(parts[7]),
                                   std::stoi(parts[2]),
                                   std::stoi(parts[3]),
                                   parseIds(parts[4]),
                                   DateTime::fromUnix(std::stoll(parts[5])));
            }
        } catch (const std::exception&) {
            // Один пошкоджений запис не повинен ламати весь файл — пропускаємо.
        }
    }
    return true;
}

std::size_t NewsRepository::authorCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return authors_.size();
}

std::size_t NewsRepository::newsCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return news_.size();
}

} // namespace news
