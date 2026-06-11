#include "news/SearchCriteria.h"
#include "news/News.h"

#include <algorithm>
#include <cctype>

namespace news {

namespace {
std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(
        std::tolower(static_cast<unsigned char>(c)));
    return s;
}
bool containsCI(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}
} // namespace

bool SearchCriteria::empty() const {
    return !titleContains && !categoryId && !tagId && !authorId && !from && !to;
}

bool SearchCriteria::matches(const News& n) const {
    if (titleContains && !containsCI(n.title(), *titleContains)) return false;
    if (categoryId && n.categoryId() != *categoryId) return false;
    if (authorId   && n.authorId()   != *authorId)   return false;
    if (tagId      && !n.hasTag(*tagId))             return false;
    if (from       && n.publishedAt() < *from)       return false;
    if (to         && n.publishedAt() > *to)         return false;
    return true;
}

} // namespace news
