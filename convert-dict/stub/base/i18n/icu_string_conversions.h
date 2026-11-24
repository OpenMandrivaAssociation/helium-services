#ifndef BASE_I18N_ICU_STRING_CONVERSIONS_H_
#define BASE_I18N_ICU_STRING_CONVERSIONS_H_

#include <string>
#include <string_view>

namespace base {

class OnStringConversionError {
 public:
  enum Type { FAIL, SKIP, SUBSTITUTE };

 private:
  OnStringConversionError() = delete;
};

bool CodepageToUTF16(std::string_view encoded,
                     const char* codepage_name,
                     OnStringConversionError::Type on_error,
                     std::u16string* utf16);

}  // namespace base

#endif  // BASE_I18N_ICU_STRING_CONVERSIONS_H_
