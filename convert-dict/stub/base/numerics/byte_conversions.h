#ifndef BASE_NUMERICS_BYTE_CONVERSIONS_H_
#define BASE_NUMERICS_BYTE_CONVERSIONS_H_

#include <stdint.h>

#include "base/containers/span.h"

namespace base {

template <typename T, size_t Extent>
inline uint16_t U16FromLittleEndian(span<T, Extent> bytes) {
  return static_cast<uint16_t>(static_cast<uint8_t>(bytes[0])) |
         (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8);
}

template <typename T, size_t Extent>
inline uint32_t U32FromLittleEndian(span<T, Extent> bytes) {
  return static_cast<uint32_t>(static_cast<uint8_t>(bytes[0])) |
         (static_cast<uint32_t>(static_cast<uint8_t>(bytes[1])) << 8) |
         (static_cast<uint32_t>(static_cast<uint8_t>(bytes[2])) << 16) |
         (static_cast<uint32_t>(static_cast<uint8_t>(bytes[3])) << 24);
}

}  // namespace base

#endif  // BASE_NUMERICS_BYTE_CONVERSIONS_H_
