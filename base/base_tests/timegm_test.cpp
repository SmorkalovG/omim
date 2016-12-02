#include "testing/testing.hpp"

#include "std/ctime.hpp"

#include <sstream>
#include <iomanip>

#include "base/timegm.hpp"
#include <time.h>

#if (defined(WIN32) || defined(_WIN32) || defined (WINDOWS) || defined (_WINDOWS))
extern "C" char* strptime(const char* s,
                          const char* f,
                          struct tm* tm) {
  std::istringstream input(s);
  input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
  input >> std::get_time(tm, f);
  if (input.fail()) {
    return nullptr;
  }
  return (char*)(s + input.tellg());
}

static time_t timegm(struct tm * a_tm)
{
    time_t ltime = mktime(a_tm);
    struct tm tm_val;
    gmtime_s(&tm_val, &ltime);
    int offset = (tm_val.tm_hour - a_tm->tm_hour);
    if (offset > 12)
    {
        offset = 24 - offset;
    }
    time_t utc = mktime(a_tm) - offset * 3600;
    return utc;
}
#endif

UNIT_TEST(TimegmTest)
{
  std::tm tm1{};
  std::tm tm2{};

  TEST(strptime("2016-05-17 07:10", "%Y-%m-%d %H:%M", &tm1), ());
  TEST(strptime("2016-05-17 07:10", "%Y-%m-%d %H:%M", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(2016, 5, 17, 7, 10, 0), ());

  TEST(strptime("2016-03-12 11:10", "%Y-%m-%d %H:%M", &tm1), ());
  TEST(strptime("2016-03-12 11:10", "%Y-%m-%d %H:%M", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(2016, 3, 12, 11, 10, 0), ());

  TEST(strptime("1970-01-01 00:00", "%Y-%m-%d %H:%M", &tm1), ());
  TEST(strptime("1970-01-01 00:00", "%Y-%m-%d %H:%M", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(1970, 1, 1, 0, 0, 0), ());

  TEST(strptime("2012-12-02 21:08:34", "%Y-%m-%d %H:%M:%S", &tm1), ());
  TEST(strptime("2012-12-02 21:08:34", "%Y-%m-%d %H:%M:%S", &tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(tm2), ());
  TEST_EQUAL(timegm(&tm1), base::TimeGM(2012, 12, 2, 21, 8, 34), ());
}
