#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <string>

namespace base {

enum LogSeverity { INFO, WARNING, ERROR, FATAL, DFATAL };

class LogMessage {
 public:
  explicit LogMessage(LogSeverity severity) : severity_(severity) {}

  ~LogMessage() {
    std::cerr << stream_.str() << std::endl;
    if (severity_ == FATAL || severity_ == DFATAL) {
      abort();
    }
  }

  std::ostream& stream() { return stream_; }

 private:
  LogSeverity severity_;
  std::ostringstream stream_;
};

}  // namespace base

#define LOG(severity) \
  ::base::LogMessage(::base::severity).stream()

#endif  // BASE_LOGGING_H_
