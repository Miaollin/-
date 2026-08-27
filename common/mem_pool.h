#pragma once

#include <cstddef>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "macros.h"

namespace Common {
template<typename T>
class MemPool final {
private:
  struct ObjectBlock {
    T object_{};
    bool is_free_ = true;
  };

  std::vector<ObjectBlock> store_;
  size_t next_free_index_ = 0;

  void updateNextFreeIndex() noexcept {
    const auto initial_free_index = next_free_index_;
    do {
      next_free_index_ = (next_free_index_ + 1) % store_.size();
      if (store_[next_free_index_].is_free_)
        return;
    } while (initial_free_index != next_free_index_);
  }

public:
  explicit MemPool(std::size_t num_elems) : store_(num_elems) {
    ASSERT(num_elems > 0, "Memory pool size must be greater than zero");
  }
  MemPool() = delete;
  MemPool(const MemPool &) = delete;
  MemPool(MemPool &&) = delete;
  MemPool &operator=(const MemPool &) = delete;
  MemPool &operator=(MemPool &&) = delete;

  template<typename... Args>
  T *allocate(Args &&...args) noexcept {
    auto *obj_block = &store_[next_free_index_];
    ASSERT(obj_block->is_free_, "Memory pool out of space");
    T *result = &obj_block->object_;
    result->~T();
    result = new(result) T(std::forward<Args>(args)...);
    obj_block->is_free_ = false;
    updateNextFreeIndex();
    return result;
  }

  void deallocate(T *elem) noexcept {
    const auto elem_index = reinterpret_cast<ObjectBlock *>(elem) - store_.data();
    ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(),
           "Element does not belong to this memory pool");
    ASSERT(!store_[elem_index].is_free_, "Element is already free");
    store_[elem_index].is_free_ = true;
    next_free_index_ = static_cast<size_t>(elem_index);
  }
};
}
