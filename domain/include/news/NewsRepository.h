#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "news/Author.h"
#include "news/Category.h"
#include "news/News.h"
#include "news/SearchCriteria.h"
#include "news/Tag.h"

namespace news {

// Сховище всіх сутностей. Доступ синхронізовано мьютексом.
class NewsRepository {
public:
    NewsRepository();

    // ---- Автори -------------------------------------------------------
    int registerAuthor(const std::string& username,
                       const std::string& email,
                       const std::string& passwordHash,
                       const std::string& bio = {});

    bool removeAuthor(int id);
    bool updateAuthor(int id, const std::string& email, const std::string& bio);

    const Author* findAuthor(int id) const;
    const Author* findAuthorByUsername(const std::string& username) const;
    std::vector<const Author*> allAuthors() const;

    // ---- Рубрики ------------------------------------------------------
    int addCategory(const std::string& name, const std::string& description = {});
    bool removeCategory(int id);
    bool updateCategory(int id, const std::string& name, const std::string& description);
    const Category* findCategory(int id) const;
    const Category* findCategoryBySlug(const std::string& slug) const;
    std::vector<Category> allCategories() const;

    // ---- Теги ---------------------------------------------------------
    int addTag(const std::string& name);
    bool removeTag(int id);
    const Tag* findTag(int id) const;
    const Tag* findTagBySlug(const std::string& slug) const;
    int ensureTag(const std::string& name);   // знайти або створити
    std::vector<Tag> allTags() const;

    // ---- Новини -------------------------------------------------------
    int addNews(int authorId,
                const std::string& title,
                const std::string& content,
                int categoryId,
                const std::vector<int>& tagIds,
                DateTime publishedAt = DateTime::now());

    bool removeNews(int id);

    bool replaceNews(int id,
                     const std::string& title,
                     const std::string& content,
                     int categoryId,
                     const std::vector<int>& tagIds);

    std::optional<News> findNews(int id) const;
    std::vector<News> allNews() const;

    std::vector<News> search(const SearchCriteria& criteria,
                             SortBy sortBy = SortBy::Date,
                             SortDirection direction = SortDirection::Desc) const;

    // ---- Файлове сховище ---------------------------------------------
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    std::size_t authorCount() const;
    std::size_t newsCount() const;

private:
    mutable std::mutex mutex_;

    std::vector<std::unique_ptr<Author>> authors_;
    std::vector<Category> categories_;
    std::vector<Tag> tags_;
    std::vector<News> news_;

    int nextAuthorId_ = 1;
    int nextCategoryId_ = 1;
    int nextTagId_ = 1;
    int nextNewsId_ = 1;

    // Без захоплення мьютекса — викликати лише з-під lock_guard.
    const Author* findAuthorUnlocked(int id) const;
    const Category* findCategoryUnlocked(int id) const;
    const Tag* findTagUnlocked(int id) const;
    News* findNewsUnlocked(int id);
    const News* findNewsUnlocked(int id) const;
};

} // namespace news
