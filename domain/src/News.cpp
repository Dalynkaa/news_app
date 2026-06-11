#include "news/News.h"

#include <algorithm>

namespace news {

News::News(int id,
           std::string title,
           std::string content,
           int authorId,
           int categoryId,
           std::vector<int> tagIds,
           DateTime publishedAt)
    : id_(id),
      title_(std::move(title)),
      content_(std::move(content)),
      authorId_(authorId),
      categoryId_(categoryId),
      tagIds_(std::move(tagIds)),
      publishedAt_(publishedAt) {}

void News::addTag(int tagId) {
    if (!hasTag(tagId)) tagIds_.push_back(tagId);
}

bool News::removeTag(int tagId) {
    auto it = std::find(tagIds_.begin(), tagIds_.end(), tagId);
    if (it == tagIds_.end()) return false;
    tagIds_.erase(it);
    return true;
}

bool News::hasTag(int tagId) const {
    return std::find(tagIds_.begin(), tagIds_.end(), tagId) != tagIds_.end();
}

std::string News::excerpt(std::size_t maxChars) const {
    // Просте обрізання за байтами з урахуванням UTF-8 (не ріжемо багатобайтовий символ).
    if (content_.size() <= maxChars) return content_;
    std::size_t cut = maxChars;
    // якщо потрапили на середину UTF-8 послідовності — здвинемо назад
    while (cut > 0 && (static_cast<unsigned char>(content_[cut]) & 0xC0) == 0x80) --cut;
    return content_.substr(0, cut) + "…";
}

} // namespace news
