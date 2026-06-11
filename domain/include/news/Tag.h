#pragma once

#include <string>

namespace news {

class Tag {
public:
    Tag(int id, std::string name);

    int id() const { return id_; }
    const std::string& name() const { return name_; }
    void setName(std::string v) { name_ = std::move(v); }

    std::string slug() const;

    bool operator==(const Tag& other) const { return id_ == other.id_; }

private:
    int id_;
    std::string name_;
};

} // namespace news
