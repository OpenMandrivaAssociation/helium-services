#ifndef CRYPTO_OBSOLETE_MD5_H_
#define CRYPTO_OBSOLETE_MD5_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <string_view>

#include "base/containers/span.h"

namespace crypto::obsolete {

inline constexpr size_t kMd5Size = 16;

class Md5 {
 public:
  static constexpr size_t kSize = kMd5Size;

  static std::array<uint8_t, kSize> Hash(std::string_view data);
  static std::array<uint8_t, kSize> Hash(base::span<const uint8_t> data);
};

}  // namespace crypto::obsolete

#endif  // CRYPTO_OBSOLETE_MD5_H_
