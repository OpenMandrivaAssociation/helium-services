#ifndef BASE_FILES_FILE_PATH_H_
#define BASE_FILES_FILE_PATH_H_

#include <string>
#include <string_view>
#include <utility>

namespace base {

class FilePath {
 public:
  FilePath() = default;
  explicit FilePath(std::string path) : path_(std::move(path)) {}
  explicit FilePath(const char* path) : path_(path ? path : "") {}

  bool empty() const { return path_.empty(); }
  const std::string& value() const { return path_; }

  // Replaces the final extension. An extension without a leading dot gets one.
  FilePath ReplaceExtension(std::string_view extension) const;

 private:
  std::string path_;
};

}  // namespace base

#endif  // BASE_FILES_FILE_PATH_H_
