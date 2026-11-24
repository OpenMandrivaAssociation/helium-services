#ifndef BASE_CONTAINERS_SPAN_H_
#define BASE_CONTAINERS_SPAN_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace base {

inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

template <typename T, size_t Extent = dynamic_extent>
class span;

template <typename Cont>
concept SpanLike = requires { std::remove_cvref_t<Cont>::extent; };

template <typename T, size_t Extent>
class span {
 public:
  using element_type = T;
  using pointer = T*;
  using size_type = size_t;

  static constexpr size_t extent = Extent;

  constexpr span() noexcept : data_(nullptr), size_(0) {
    static_assert(Extent == dynamic_extent || Extent == 0,
                  "a fixed-size span needs a pointer");
  }

  constexpr span(T* ptr, size_t count) noexcept : data_(ptr), size_(count) {}

  template <typename Cont>
    requires(!SpanLike<Cont> &&
             requires(Cont& container) {
               { container.data() } -> std::convertible_to<T*>;
               { container.size() } -> std::convertible_to<size_t>;
             })
  constexpr span(Cont& container) noexcept
      : data_(container.data()), size_(container.size()) {}

  template <typename U, size_t N>
    requires(std::is_convertible_v<U*, T*> &&
             !(std::is_same_v<U, T> && N == Extent))
  constexpr span(const span<U, N>& other) noexcept
      : data_(other.data()), size_(other.size()) {}

  constexpr T* data() const noexcept { return data_; }
  constexpr size_t size() const noexcept { return size_; }
  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr T& operator[](size_t index) const noexcept { return data_[index]; }

  template <size_t Count>
  span<T, Count> first() const {
    if (Count > size_) {
      fprintf(stderr, "span::first out of range\n");
      abort();
    }
    return span<T, Count>(data_, Count);
  }

  span<T> first(size_t count) const {
    if (count > size_) {
      fprintf(stderr, "span::first out of range\n");
      abort();
    }
    return span<T>(data_, count);
  }

  span<T> subspan(size_t offset) const {
    if (offset > size_) {
      fprintf(stderr, "span::subspan out of range\n");
      abort();
    }
    return span<T>(data_ + offset, size_ - offset);
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  auto subspan() const {
    if (Offset > size_) {
      fprintf(stderr, "span::subspan out of range\n");
      abort();
    }
    if constexpr (Count != dynamic_extent) {
      if (Offset + Count > size_) {
        fprintf(stderr, "span::subspan out of range\n");
        abort();
      }
      return span<T, Count>(data_ + Offset, Count);
    } else if constexpr (Extent != dynamic_extent) {
      return span<T, Extent - Offset>(data_ + Offset, size_ - Offset);
    } else {
      return span<T>(data_ + Offset, size_ - Offset);
    }
  }

  template <size_t Count>
  std::pair<span<T, Count>, span<T>> split_at() const {
    return {first<Count>(), subspan(Count)};
  }

  template <typename U, size_t N>
  void copy_from(const span<U, N>& other) const {
    if (size_ != other.size()) {
      fprintf(stderr, "span::copy_from size mismatch\n");
      abort();
    }
    if (size_ != 0) {
      memcpy(data_, other.data(), size_ * sizeof(T));
    }
  }

 private:
  T* data_;
  size_t size_;
};

inline span<const uint8_t> as_byte_span(std::string_view bytes) {
  return span<const uint8_t>(reinterpret_cast<const uint8_t*>(bytes.data()),
                             bytes.size());
}

inline span<const uint8_t> as_byte_span(const std::string& bytes) {
  return as_byte_span(std::string_view(bytes));
}

template <typename T>
span<uint8_t> byte_span_from_ref(T& object) {
  return span<uint8_t>(reinterpret_cast<uint8_t*>(std::addressof(object)),
                       sizeof(T));
}

}  // namespace base

#endif  // BASE_CONTAINERS_SPAN_H_
