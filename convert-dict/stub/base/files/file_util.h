#ifndef BASE_FILES_FILE_UTIL_H_
#define BASE_FILES_FILE_UTIL_H_

#include <stdio.h>

#include "base/files/file_path.h"

#ifndef PRFilePath
#define PRFilePath "s"
#endif

#ifndef FILE_PATH_LITERAL
#define FILE_PATH_LITERAL(x) x
#endif

namespace base {

inline FILE* OpenFile(const FilePath& path, const char* mode) {
  return fopen(path.value().c_str(), mode);
}

inline bool CloseFile(FILE* file) {
  return file && fclose(file) == 0;
}

}  // namespace base

#endif  // BASE_FILES_FILE_UTIL_H_
