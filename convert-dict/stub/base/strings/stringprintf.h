#ifndef BASE_STRINGS_STRINGPRINTF_H_
#define BASE_STRINGS_STRINGPRINTF_H_

#include <stdio.h>

#include <string>

namespace base {

template <typename... Args>
std::string StringPrintf(const char* format, const Args&... args) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
  int needed = snprintf(nullptr, 0, format, args...);
  if (needed < 0) {
    return std::string();
  }
  std::string out(static_cast<size_t>(needed), '\0');
  snprintf(out.data(), out.size() + 1, format, args...);
#pragma clang diagnostic pop
  return out;
}

}  // namespace base

#endif  // BASE_STRINGS_STRINGPRINTF_H_
