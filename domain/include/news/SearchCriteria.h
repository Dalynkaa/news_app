#pragma once

#include <optional>
#include <string>
#include "news/DateTime.h"

namespace news {

class News;

struct SearchCriteria {
    std::optional<std::string> titleContains;
    std::optional<int> categoryId;
    std::optional<int> tagId;
    std::optional<int> authorId;
    std::optional<DateTime> from;
    std::optional<DateTime> to;

    bool matches(const News& n) const;
    bool empty() const;
};

enum class SortBy { Date, Title, Category, Author };
enum class SortDirection { Asc, Desc };

} // namespace news
