
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
    {
      std::lock_guard<std::mutex> lockg{lock};
      container.emplace_back(std::move(func));
    }

    // let a waiting thread know there is an available job
    notifier.notify_one();
  }

  template <typename Func, typename Iter>
  auto for_each(Iter beg, Iter end, Func&& func)
  {
    auto last = end - 1;
    std::atomic_uint atom = (uint32_t)std::distance(beg, end);
    auto             event = Event(Event::iunset);
    for (auto i = beg; i != end; ++i)
    {
      if (i == last)
      {
        notifier.notify_all();
        func(*i);
        if (atom.fetch_sub(1) == 1)
          event.set();
      }
      else
      {
        std::lock_guard<std::mutex> lockg{lock};
        container.emplace_back([&]() {
            func(*i);
            if (atom.fetch_sub(1) == 1)
              event.set();
          });
      }
    }

    event.wait();
  }

private:
  using Entry = std::function<void()>;

  std::condition_variable  notifier;
  std::vector<std::thread> workers;
  std::mutex               lock;
  std::deque<Entry>        container;
  std::atomic_int          quit = 0;
  
};

} // namespace terra