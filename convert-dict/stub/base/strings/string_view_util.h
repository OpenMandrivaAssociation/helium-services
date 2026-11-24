#ifndef BASE_STRINGS_STRING_VIEW_UTIL_H_
#define BASE_STRINGS_STRING_VIEW_UTIL_H_

#include <string_view>

#include "base/containers/span.h"

namespace base {

inline std::string_view as_string_view(span<const uint8_t> bytes) {
  return std::string_view(reinterpret_cast<const char*>(bytes.data()),
                          bytes.size());
}

}  // namespace base

#endif  // BASE_STRINGS_STRING_VIEW_UTIL_H_
