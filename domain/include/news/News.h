#pragma once

#include <string>
#include <vector>
#include "news/DateTime.h"

namespace news {

class News {
public:
    News(int id,
         std::string title,
         std::string content,
         int authorId,
         int categoryId,
         std::vector<int> tagIds,
         DateTime publishedAt);

    int id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& content() const { return content_; }
    int authorId() const { return authorId_; }
    int categoryId() const { return categoryId_; }
    const std::vector<int>& tagIds() const { return tagIds_; }
    const DateTime& publishedAt() const { return publishedAt_; }

    void setTitle(std::string v) { title_ = std::move(v); }
    void setContent(std::string v) { content_ = std::move(v); }
    void setCategoryId(int v) { categoryId_ = v; }
    void setTagIds(std::vector<int> v) { tagIds_ = std::move(v); }
    void setPublishedAt(DateTime v) { publishedAt_ = v; }

    void addTag(int tagId);
    bool removeTag(int tagId);
    bool hasTag(int tagId) const;

    std::string excerpt(std::size_t maxChars = 200) const;

private:
    int id_;
    std::string title_;
    std::string content_;
    int authorId_;
    int categoryId_;
    std::vector<int> tagIds_;
    DateTime publishedAt_;
};

} // namespace news
