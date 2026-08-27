#pragma once

#include <cstdlib>
#include <iostream>
#include <string>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

inline void ASSERT(bool cond, const std::string &msg) noexcept {
  if (UNLIKELY(!cond)) {
    std::cerr << msg << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

inline void FATAL(const std::string &msg) noexcept {
  std::cerr << msg << std::endl;
  std::exit(EXIT_FAILURE);
}
