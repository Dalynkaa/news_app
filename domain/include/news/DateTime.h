#pragma once

#include <string>
#include <cstdint>

namespace news {

class DateTime {
public:
    DateTime();                                   // поточний момент
    DateTime(int year, int month, int day,
             int hour = 0, int minute = 0, int second = 0);

    static DateTime now();
    static DateTime fromUnix(std::int64_t seconds);

    // Розбір ISO-8601 ("YYYY-MM-DD" або "YYYY-MM-DDTHH:MM:SS").
    static DateTime parse(const std::string& iso);

    std::int64_t toUnix() const { return seconds_; }

    int year()   const;
    int month()  const;
    int day()    const;
    int hour()   const;
    int minute() const;
    int second() const;

    std::string toIso()  const;   // "YYYY-MM-DDTHH:MM:SS"
    std::string toDate() const;   // "YYYY-MM-DD"
    std::string toHuman() const;  // "15 травня 2026, 14:32"

    bool operator==(const DateTime& other) const { return seconds_ == other.seconds_; }
    bool operator!=(const DateTime& other) const { return seconds_ != other.seconds_; }
    bool operator< (const DateTime& other) const { return seconds_ <  other.seconds_; }
    bool operator<=(const DateTime& other) const { return seconds_ <= other.seconds_; }
    bool operator> (const DateTime& other) const { return seconds_ >  other.seconds_; }
    bool operator>=(const DateTime& other) const { return seconds_ >= other.seconds_; }

private:
    std::int64_t seconds_;  // Unix timestamp UTC
};

} // namespace news
