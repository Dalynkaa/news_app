#include "news/Tag.h"
#include "news/Category.h"

namespace news {

Tag::Tag(int id, std::string name) : id_(id), name_(std::move(name)) {}

std::string Tag::slug() const {
    return Category(0, name_).slug();
}

} // namespace news
