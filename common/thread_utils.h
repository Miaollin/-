#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <unistd.h>
#include <sys/syscall.h>

namespace Common {
inline bool setThreadCore(int core_id) noexcept {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
}

template<typename T, typename... A>
inline auto createAndStartThread(int core_id, const std::string &name, T &&func, A &&...args) noexcept {
  std::atomic<bool> running{false}, failed{false};
  auto thread_body = [&] {
    if (core_id >= 0 && !setThreadCore(core_id)) {
      std::cerr << "Failed to set core affinity for " << name << " " << syscall(SYS_gettid)
                << " to " << core_id << std::endl;
      failed = true;
      return;
    }
    std::cout << "Set core affinity for " << name << " " << syscall(SYS_gettid)
              << " " << core_id << std::endl;
    running = true;
    std::forward<T>(func)(std::forward<A>(args)...);
  };
  auto thread = new std::thread(thread_body);

  while (!running && !failed)
    std::this_thread::yield();

  if (failed) {
    thread->join();
    delete thread;
    thread = nullptr;
  }
  return thread;
}
}
