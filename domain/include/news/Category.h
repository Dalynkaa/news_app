#pragma once

#include <string>

namespace news {

class Category {
public:
    Category(int id, std::string name, std::string description = {});

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }

    void setName(std::string v) { name_ = std::move(v); }
    void setDescription(std::string v) { description_ = std::move(v); }

    std::string slug() const;   // "Спорт" -> "sport"

    bool operator==(const Category& other) const { return id_ == other.id_; }

private:
    int id_;
    std::string name_;
    std::string description_;
};

} // namespace news
