#pragma once

#include <atomic>
#include <cstddef>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

#include "macros.h"

namespace Common {
template<typename T>
class LFQueue final {
private:
  std::vector<T> store_;
  std::atomic<size_t> next_write_index_{0};
  std::atomic<size_t> next_read_index_{0};
  std::atomic<size_t> num_elements_{0};

public:
  explicit LFQueue(std::size_t num_elems) : store_(num_elems, T()) {
    ASSERT(num_elems > 0, "LFQueue size must be greater than zero");
  }
  LFQueue() = delete;
  LFQueue(const LFQueue &) = delete;
  LFQueue(LFQueue &&) = delete;
  LFQueue &operator=(const LFQueue &) = delete;
  LFQueue &operator=(LFQueue &&) = delete;

  auto getNextToWrite() noexcept -> T * {
    return &store_[next_write_index_.load()];
  }

  void updateWriteIndex() noexcept {
    ASSERT(num_elements_.load() < store_.size(), "LFQueue is full");
    next_write_index_ = (next_write_index_.load() + 1) % store_.size();
    num_elements_++;
  }

  auto getNextToRead() const noexcept -> const T * {
    return num_elements_.load() == 0 ? nullptr : &store_[next_read_index_.load()];
  }

  void updateReadIndex() noexcept {
    ASSERT(num_elements_.load() != 0,
           "Read an invalid element in:" + std::to_string(syscall(SYS_gettid)));
    next_read_index_ = (next_read_index_.load() + 1) % store_.size();
    num_elements_--;
  }

  auto size() const noexcept -> size_t {
    return num_elements_.load();
  }
};
}
