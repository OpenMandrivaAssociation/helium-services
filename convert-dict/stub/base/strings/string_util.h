#ifndef BASE_STRINGS_STRING_UTIL_H_
#define BASE_STRINGS_STRING_UTIL_H_

#include <string>
#include <string_view>

#include "base/containers/span.h"

namespace base {

enum TrimPositions {
  TRIM_NONE = 0,
  TRIM_LEADING = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL = TRIM_LEADING | TRIM_TRAILING,
};

// ASCII whitespace only: tab, vertical tab, form feed, space, LF, CR.
TrimPositions TrimWhitespaceASCII(std::string_view input,
                                  TrimPositions positions,
                                  std::string* output);

void ReplaceSubstringsAfterOffset(std::string* str,
                                  size_t start_offset,
                                  std::string_view find_this,
                                  std::string_view replace_with);

inline std::string JoinString(span<const std::string> parts,
                              std::string_view separator) {
  std::string out;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) {
      out.append(separator);
    }
    out.append(parts[i]);
  }
  return out;
}

}  // namespace base

#endif  // BASE_STRINGS_STRING_UTIL_H_
