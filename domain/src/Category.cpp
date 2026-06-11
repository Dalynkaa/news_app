#include "news/Category.h"

#include <cctype>
#include <string>
#include <unordered_map>

namespace news {

namespace {

// Транслітерація українських літер у латиницю для slug.
const std::unordered_map<std::string, std::string>& translitMap() {
    static const std::unordered_map<std::string, std::string> m = {
        {"а","a"},{"б","b"},{"в","v"},{"г","h"},{"ґ","g"},{"д","d"},
        {"е","e"},{"є","ie"},{"ж","zh"},{"з","z"},{"и","y"},{"і","i"},
        {"ї","i"},{"й","i"},{"к","k"},{"л","l"},{"м","m"},{"н","n"},
        {"о","o"},{"п","p"},{"р","r"},{"с","s"},{"т","t"},{"у","u"},
        {"ф","f"},{"х","kh"},{"ц","ts"},{"ч","ch"},{"ш","sh"},{"щ","shch"},
        {"ь",""},{"ю","iu"},{"я","ia"},
    };
    return m;
}


std::string nextCodepointLower(const std::string& s, std::size_t& i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    std::size_t len = 1;
    if      ((c & 0x80) == 0x00) len = 1;
    else if ((c & 0xE0) == 0xC0) len = 2;
    else if ((c & 0xF0) == 0xE0) len = 3;
    else if ((c & 0xF8) == 0xF0) len = 4;

    std::string out = s.substr(i, len);
    i += len;

    if (len == 1) {
        out[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[0])));
    } else if (len == 2) {
        // Кирилиця: великі літери знаходяться у блоці 0x0410..0x042F (А..Я)
        // у UTF-8: 0xD0 0x90..0xBF та 0xD1 0x80..0x8F. Перетворення = +0x20 у коді.
        unsigned char b0 = static_cast<unsigned char>(out[0]);
        unsigned char b1 = static_cast<unsigned char>(out[1]);
        if (b0 == 0xD0 && b1 >= 0x90 && b1 <= 0x9F) {
            // А..П -> а..п (D0 90..9F -> D0 B0..BF)
            out[1] = static_cast<char>(b1 + 0x20);
        } else if (b0 == 0xD0 && b1 >= 0xA0 && b1 <= 0xAF) {
            // Р..Я -> р..я (D0 A0..AF -> D1 80..8F)
            out[0] = static_cast<char>(0xD1);
            out[1] = static_cast<char>(b1 - 0x20);
        } else if (b0 == 0xD0 && b1 == 0x81) {
            out[1] = static_cast<char>(0x91); // Ё -> ё
        } else if (b0 == 0xD0 && b1 == 0x86) {
            out[0] = static_cast<char>(0xD1);
            out[1] = static_cast<char>(0x96); // І -> і
        } else if (b0 == 0xD0 && b1 == 0x84) {
            out[0] = static_cast<char>(0xD1);
            out[1] = static_cast<char>(0x94); // Є -> є
        } else if (b0 == 0xD0 && b1 == 0x87) {
            out[0] = static_cast<char>(0xD1);
            out[1] = static_cast<char>(0x97); // Ї -> ї
        } else if (b0 == 0xD2 && b1 == 0x90) {
            out[1] = static_cast<char>(0x91); // Ґ -> ґ
        }
    }
    return out;
}

std::string slugify(const std::string& s) {
    const auto& map = translitMap();
    std::string out;
    out.reserve(s.size());
    std::size_t i = 0;
    bool prevDash = true; // не дозволяти стартовий "-"
    while (i < s.size()) {
        std::string cp = nextCodepointLower(s, i);
        if (cp.size() == 1) {
            char c = cp[0];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                out += c;
                prevDash = false;
            } else if (!prevDash) {
                out += '-';
                prevDash = true;
            }
        } else {
            auto it = map.find(cp);
            if (it != map.end()) {
                if (it->second.empty()) {
                    // м'який знак тощо — пропускаємо
                } else {
                    out += it->second;
                    prevDash = false;
                }
            } else if (!prevDash) {
                out += '-';
                prevDash = true;
            }
        }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    if (out.empty()) out = "n";
    return out;
}

} // namespace

Category::Category(int id, std::string name, std::string description)
    : id_(id), name_(std::move(name)), description_(std::move(description)) {}

std::string Category::slug() const {
    return slugify(name_);
}

} // namespace news
