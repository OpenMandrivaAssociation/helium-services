#ifndef BASE_COMPILER_SPECIFIC_H_
#define BASE_COMPILER_SPECIFIC_H_

#define UNSAFE_TODO(expr) (expr)
#define UNSAFE_BUFFERS(...) __VA_ARGS__
#define LIFETIME_BOUND
#define PRINTF_FORMAT(fmt, args)

#endif  // BASE_COMPILER_SPECIFIC_H_
