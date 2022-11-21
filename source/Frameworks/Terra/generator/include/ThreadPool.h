
#pragma once

#include "Common.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <latch>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>

namespace terra
{
template <typename It>
struct block_range
{
  It begin() const
  {
    return begin_;
  }

  It end() const
  {
    return end_;
  }

  block_range(It b, size_t c) : begin_(b), end_((It)(b + c)) {}

  It begin_;
  It end_;
};

constexpr bool SingleThreadedDebug = false;

class ThreadPool
{

public:
  ThreadPool();

  using Task = std::function<void()>;
  static ThreadPool& get();

  void shutdown();
  void cancel();

  auto addTask(Task&& func)
  {
    {
      std::lock_guard<std::mutex> lockg{lock};
      container.emplace_back(std::move(func));
    }

    // let a waiting thread know there is an available job
    notifier.notify_one();
  }

  template <typename Iter, typename L>
  auto for_each(Iter beg, Iter end, L&& lambda)
  {
    if constexpr (SingleThreadedDebug)
    {
      for (auto i = beg; i != end; ++i)
      {
        if constexpr (std::is_integral_v<Iter>)
          lambda(i);
        else
          lambda(*i);
      }
      return;
    }
    auto           last = end - 1;
    std::ptrdiff_t nb   = 0;

    if constexpr (std::is_integral_v<Iter>)
      nb = static_cast<std::ptrdiff_t>(end - beg);
    else
      nb = std::distance(beg, end);

    if (nb <= 0)
      return;

    if (nb == 1)
    {
      if constexpr (std::is_integral_v<Iter>)
        lambda(beg);
      else
        lambda(*beg);
      return;
    }

    auto l = std::latch(nb - 1);
    for (auto i = beg; i != end; ++i)
    {
      if (i == last)
      {
        notifier.notify_all();
        if constexpr (std::is_integral_v<Iter>)
          lambda(i);
        else
          lambda(*i);
        l.wait();
      }
      else
      {

        std::lock_guard<std::mutex> lockg{lock};
        if constexpr (std::is_integral_v<Iter>)
        {
          container.emplace_back(
            [i, &lambda, &l]()
            {
              lambda(i);
              l.count_down();
            });
        }
        else
        {
          auto* obj = &(*i);
          container.emplace_back(
            [obj, &lambda, &l]()
            {
              lambda(*obj);
              l.count_down();
            });
        }
      }
    }
  }

  template <typename Iter, typename L>
  auto for_each(Iter beg, Iter end, uint32_t granularity, L&& lambda)
  {
    if constexpr (SingleThreadedDebug)
    {
      for (auto i = beg; i != end; ++i)
      {
        if constexpr (std::is_integral_v<Iter>)
          lambda(i);
        else
          lambda(*i);
      }
      return;
    }

    std::ptrdiff_t nb = 0;

    if constexpr (std::is_integral_v<Iter>)
      nb = static_cast<std::ptrdiff_t>(end - beg);
    else
      nb = std::distance(beg, end);

    if (nb > 0)
    {
      auto taskCnt = (nb + (granularity - 1)) / granularity;
      if (taskCnt == 1)
      {
        for (auto i = beg; i != end; ++i)
        {
          if constexpr (std::is_integral_v<Iter>)
            lambda(i);
          else
            lambda(*i);
        }
      }
      else
      {
        auto     l     = std::latch(taskCnt - 1);
        uint32_t start = 0;
        auto     last  = taskCnt - 1;
        for (uint32 t = 0; t < taskCnt; ++t, start += granularity)
        {
          if (t == last)
          {
            notifier.notify_all();
            auto range = block_range(beg + start, nb - start);
            for (auto it = range.begin(); it != range.end(); ++it)
            {
              if constexpr (std::is_integral_v<Iter>)
                lambda(it);
              else
                lambda(*it);
            }

            l.wait();
          }
          else
          {
            std::lock_guard<std::mutex> lockg{lock};
            container.emplace_back(
              [&lambda, &l, range = block_range(beg + start, granularity)]()
              {
                for (auto it = range.begin(); it != range.end(); ++it)
                {
                  if constexpr (std::is_integral_v<Iter>)
                    lambda(it);
                  else
                    lambda(*it);
                }
                l.count_down();
              });
          }
        }
      }
    }
  }

private:
  std::condition_variable  notifier;
  std::vector<std::thread> workers;
  std::mutex               lock;
  std::deque<Task>         container;
  std::atomic_bool         quit = 0;
};

} // namespace terra
