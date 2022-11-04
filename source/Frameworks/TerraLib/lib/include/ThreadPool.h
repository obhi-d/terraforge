
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include "Common.h"

namespace terra
{
using Task = std::packaged_task<void()>;


class ThreadPool
{
public:

  ThreadPool();
  
  static ThreadPool& get();

  void shutdown();
  void cancel();

  template <typename Func>
  auto add(Func&& func) 
  {
    std::future<void> f;
    {
      std::lock_guard<std::mutex> lockg{lock};
      container.emplace_back(std::move(func));
      f = container.back().get_future();
    }

    // let a waiting thread know there is an available job
    notifier.notify_one();
    return f;
  }

  template <typename Func, typename Iter>
  auto for_each(Iter beg, Iter end, Func&& func, WaitList& waiters)
  {
    auto last = end - 1;
    auto nb = std::distance(beg, end);
    if (nb > 0)
    {
      waiters.resize(nb - 1);
      uint32_t task = 0;
      for (auto i = beg; i != end; ++i, ++task)
      {
        if (i == last)
        {
          notifier.notify_all();
          func(*i);
        }
        else
        {
          std::lock_guard<std::mutex> lockg{lock};
          auto* obj = &(*i);
          container.emplace_back([obj, &func]() {
                        func(*obj);
                      });
          waiters[task] = container.back().get_future();
        }
      }
      for(auto& w : waiters)
        w.wait();
    }
  }

private:
  using Entry = std::packaged_task<void()>;

  std::condition_variable  notifier;
  std::vector<std::thread> workers;
  std::mutex               lock;
  std::deque<Entry>        container;
  std::atomic_int          quit = 0;
  
};

} // namespace terra
