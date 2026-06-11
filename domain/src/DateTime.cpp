#include "news/DateTime.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <stdexcept>

namespace news {

namespace {

std::tm toTmUtc(std::int64_t seconds) {
    std::time_t t = static_cast<std::time_t>(seconds);
    std::tm out{};
#if defined(_WIN32)
    gmtime_s(&out, &t);
#else
    gmtime_r(&t, &out);
#endif
    return out;
}

std::int64_t fromTmUtc(int year, int month, int day, int hour, int minute, int second) {
    std::tm t{};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;
    t.tm_isdst = 0;
#if defined(_WIN32)
    return _mkgmtime(&t);
#else
    return static_cast<std::int64_t>(timegm(&t));
#endif
}

} // namespace

DateTime::DateTime() : seconds_(static_cast<std::int64_t>(std::time(nullptr))) {}

DateTime::DateTime(int year, int month, int day, int hour, int minute, int second)
    : seconds_(fromTmUtc(year, month, day, hour, minute, second)) {}

DateTime DateTime::now() {
    return DateTime();
}

DateTime DateTime::fromUnix(std::int64_t seconds) {
    DateTime dt;
    dt.seconds_ = seconds;
    return dt;
}

DateTime DateTime::parse(const std::string& iso) {
    // Підтримуємо: "YYYY-MM-DD" або "YYYY-MM-DDTHH:MM" або "YYYY-MM-DDTHH:MM:SS".
    int Y = 0, M = 0, D = 0, h = 0, m = 0, s = 0;
    if (iso.size() < 10) {
        throw std::invalid_argument("DateTime::parse: рядок занадто короткий: " + iso);
    }
    int n = std::sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &Y, &M, &D, &h, &m, &s);
    if (n < 3) {
        // спробуємо з пробілом замість T
        n = std::sscanf(iso.c_str(), "%d-%d-%d %d:%d:%d", &Y, &M, &D, &h, &m, &s);
    }
    if (n < 3) {
        throw std::invalid_argument("DateTime::parse: неправильний формат: " + iso);
    }
    return DateTime(Y, M, D, h, m, s);
}

int DateTime::year()   const { return toTmUtc(seconds_).tm_year + 1900; }
int DateTime::month()  const { return toTmUtc(seconds_).tm_mon + 1; }
int DateTime::day()    const { return toTmUtc(seconds_).tm_mday; }
int DateTime::hour()   const { return toTmUtc(seconds_).tm_hour; }
int DateTime::minute() const { return toTmUtc(seconds_).tm_min; }
int DateTime::second() const { return toTmUtc(seconds_).tm_sec; }

std::string DateTime::toIso() const {
    auto t = toTmUtc(seconds_);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec);
    return buf;
}

std::string DateTime::toDate() const {
    auto t = toTmUtc(seconds_);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    return buf;
}

std::string DateTime::toHuman() const {
    static const char* months[] = {
        "січня",  "лютого", "березня", "квітня",  "травня",   "червня",
        "липня",  "серпня", "вересня", "жовтня",  "листопада", "грудня"};
    auto t = toTmUtc(seconds_);
    char buf[64];
    int mi = t.tm_mon;
    if (mi < 0 || mi > 11) mi = 0;
    std::snprintf(buf, sizeof(buf), "%d %s %d, %02d:%02d",
                  t.tm_mday, months[mi], t.tm_year + 1900,
                  t.tm_hour, t.tm_min);
    return buf;
}

} // namespace news
