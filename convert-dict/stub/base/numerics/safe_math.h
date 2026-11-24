#ifndef BASE_NUMERICS_SAFE_MATH_H_
#define BASE_NUMERICS_SAFE_MATH_H_

#include <limits>
#include <type_traits>

#include "base/check.h"

namespace base {

template <typename T>
class CheckedNumeric {
 public:
  CheckedNumeric() = default;
  explicit CheckedNumeric(T value) : value_(value) {}

  CheckedNumeric& operator+=(T rhs) {
    if (!valid_) {
      return *this;
    }
    if constexpr (std::is_unsigned_v<T>) {
      if (value_ > std::numeric_limits<T>::max() - rhs) {
        valid_ = false;
      } else {
        value_ += rhs;
      }
    } else {
      value_ += rhs;
    }
    return *this;
  }

  bool IsValid() const { return valid_; }

  T ValueOrDie() const {
    CHECK(valid_);
    return value_;
  }

 private:
  T value_ = 0;
  bool valid_ = true;
};

}  // namespace base

#endif  // BASE_NUMERICS_SAFE_MATH_H_
