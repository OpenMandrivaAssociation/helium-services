#include "base/files/file_path.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "crypto/obsolete/md5.h"

#include <unicode/ucnv.h>
#include <unicode/ustring.h>

#include <openssl/evp.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace {

bool IsAsciiSpace(unsigned char c) {
  return c == '\t' || c == '\v' || c == '\f' || c == ' ' || c == '\n' ||
         c == '\r';
}

std::string IcuCharsetName(std::string name) {
  for (char& c : name) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  if (name == "UTF8" || name == "UTF-8") {
    return "UTF-8";
  }
  if (name.rfind("ISO8859-", 0) == 0) {
    return "ISO-8859-" + name.substr(7);
  }
  if (name.rfind("MICROSOFT-CP", 0) == 0) {
    return "windows-" + name.substr(12);
  }
  if (name.rfind("CP", 0) == 0 && name.size() > 2 &&
      std::isdigit(static_cast<unsigned char>(name[2]))) {
    return "windows-" + name.substr(2);
  }
  return name;
}

}  // namespace

namespace base {

FilePath FilePath::ReplaceExtension(std::string_view extension) const {
  std::string base = path_;
  const auto slash = base.find_last_of('/');
  const auto dot = base.find_last_of('.');
  if (dot != std::string::npos &&
      (slash == std::string::npos || dot > slash)) {
    base.resize(dot);
  }
  if (extension.empty() ||
      (extension.size() == 1 && extension[0] == '.')) {
    return FilePath(std::move(base));
  }
  if (extension[0] != '.') {
    base.push_back('.');
  }
  base.append(extension);
  return FilePath(std::move(base));
}

TrimPositions TrimWhitespaceASCII(std::string_view input,
                                  TrimPositions positions,
                                  std::string* output) {
  size_t begin = 0;
  size_t end = input.size();
  TrimPositions trimmed = TRIM_NONE;
  if (positions & TRIM_LEADING) {
    while (begin < end &&
           IsAsciiSpace(static_cast<unsigned char>(input[begin]))) {
      ++begin;
      trimmed = static_cast<TrimPositions>(trimmed | TRIM_LEADING);
    }
  }
  if (positions & TRIM_TRAILING) {
    while (end > begin &&
           IsAsciiSpace(static_cast<unsigned char>(input[end - 1]))) {
      --end;
      trimmed = static_cast<TrimPositions>(trimmed | TRIM_TRAILING);
    }
  }
  output->assign(input.data() + begin, end - begin);
  return trimmed;
}

void ReplaceSubstringsAfterOffset(std::string* str,
                                  size_t start_offset,
                                  std::string_view find_this,
                                  std::string_view replace_with) {
  if (find_this.empty() || start_offset > str->size()) {
    return;
  }
  size_t pos = start_offset;
  while ((pos = str->find(find_this, pos)) != std::string::npos) {
    str->replace(pos, find_this.size(), replace_with);
    pos += replace_with.size();
  }
}

std::vector<std::string> SplitString(std::string_view input,
                                     std::string_view separators,
                                     WhitespaceHandling whitespace,
                                     SplitResult result_type) {
  std::vector<std::string> result;
  if (input.empty()) {
    return result;
  }
  size_t start = 0;
  while (start != std::string::npos) {
    const size_t end = input.find_first_of(separators, start);
    std::string_view piece;
    if (end == std::string_view::npos) {
      piece = input.substr(start);
      start = std::string::npos;
    } else {
      piece = input.substr(start, end - start);
      start = end + 1;
    }
    if (whitespace == TRIM_WHITESPACE) {
      std::string trimmed;
      TrimWhitespaceASCII(piece, TRIM_ALL, &trimmed);
      piece = trimmed;
      if (result_type == SPLIT_WANT_ALL || !piece.empty()) {
        result.emplace_back(trimmed);
      }
      continue;
    }
    if (result_type == SPLIT_WANT_ALL || !piece.empty()) {
      result.emplace_back(piece);
    }
  }
  return result;
}

bool CodepageToUTF16(std::string_view encoded,
                     const char* codepage_name,
                     OnStringConversionError::Type on_error,
                     std::u16string* utf16) {
  utf16->clear();
  if (!codepage_name || !codepage_name[0]) {
    return false;
  }
  if (encoded.empty()) {
    return true;
  }

  const std::string charset = IcuCharsetName(codepage_name);
  UErrorCode err = U_ZERO_ERROR;
  UConverter* converter = ucnv_open(charset.c_str(), &err);
  if (U_FAILURE(err) || !converter) {
    return false;
  }
  if (on_error == OnStringConversionError::FAIL) {
    err = U_ZERO_ERROR;
    ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_STOP, nullptr, nullptr,
                        nullptr, &err);
    if (U_FAILURE(err)) {
      ucnv_close(converter);
      return false;
    }
  }

  const int32_t src_len = static_cast<int32_t>(encoded.size());
  int32_t capacity = src_len + 1;
  utf16->resize(static_cast<size_t>(capacity));
  err = U_ZERO_ERROR;
  int32_t written = ucnv_toUChars(
      converter, reinterpret_cast<UChar*>(utf16->data()), capacity,
      encoded.data(), src_len, &err);
  if (err == U_BUFFER_OVERFLOW_ERROR) {
    capacity = written;
    utf16->resize(static_cast<size_t>(capacity));
    err = U_ZERO_ERROR;
    written = ucnv_toUChars(converter, reinterpret_cast<UChar*>(utf16->data()),
                            capacity, encoded.data(), src_len, &err);
  }
  ucnv_close(converter);
  if (U_FAILURE(err) || written < 0) {
    utf16->clear();
    return false;
  }
  utf16->resize(static_cast<size_t>(written));
  return true;
}

std::string UTF16ToUTF8(std::u16string_view utf16) {
  if (utf16.empty()) {
    return std::string();
  }
  UErrorCode err = U_ZERO_ERROR;
  int32_t length = 0;
  u_strToUTF8(nullptr, 0, &length, reinterpret_cast<const UChar*>(utf16.data()),
              static_cast<int32_t>(utf16.size()), &err);
  if (err != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(err)) {
    return std::string();
  }
  err = U_ZERO_ERROR;
  std::string out(static_cast<size_t>(length), '\0');
  u_strToUTF8(out.data(), length, nullptr,
              reinterpret_cast<const UChar*>(utf16.data()),
              static_cast<int32_t>(utf16.size()), &err);
  if (U_FAILURE(err)) {
    return std::string();
  }
  return out;
}

}  // namespace base

namespace crypto::obsolete {

std::array<uint8_t, Md5::kSize> Md5::Hash(std::string_view data) {
  return Hash(base::as_byte_span(data));
}

std::array<uint8_t, Md5::kSize> Md5::Hash(base::span<const uint8_t> data) {
  std::array<uint8_t, kSize> digest{};
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    return digest;
  }
  unsigned int length = 0;
  const int ok = EVP_DigestInit_ex(ctx, EVP_md5(), nullptr) == 1 &&
                 EVP_DigestUpdate(ctx, data.data(), data.size()) == 1 &&
                 EVP_DigestFinal_ex(ctx, digest.data(), &length) == 1 &&
                 length == kSize;
  EVP_MD_CTX_free(ctx);
  if (!ok) {
    digest.fill(0);
  }
  return digest;
}

}  // namespace crypto::obsolete
