
#include <Terra.h>
#include<ThreadPool.h>

namespace terra
{

ThreadPool::ThreadPool()
{
  uint32_t hwc = std::thread::hardware_concurrency();
  for (uint32_t i = 0; i < hwc; ++i)
  {
    workers.emplace_back(
      [this]
      {
        while (!quit.load())
        {
          Entry entry;
          {
            std::unique_lock<std::mutex> lockg{lock};
            if (container.empty())
            {
              notifier.wait(lockg,
                            [&]()
                            {
                              return quit.load() || !(container.empty());
                            });
            }

            if (container.empty())
              continue;

            entry = std::move(container.front());
            container.pop_front();
          }

          entry();
          entry = nullptr;
        }

        quit++;
      });
  }
}

ThreadPool& ThreadPool::get()
{
  return Terra::get().pool();
}

void ThreadPool::shutdown()
{
  quit = 1;
  uint32_t rc = (uint32_t)workers.size() + 1;
  while (quit.load() != rc)
    notifier.notify_all();
  for (auto& w : workers)
  {
    if (w.joinable())
      w.join();
  }
}

void ThreadPool::cancel()
{
  auto event = Event(Event::iunset);
    
  {
    std::unique_lock<std::mutex> lockg{lock};
    container.clear();
    container.emplace_back(
      [&event]
      {
        event.set();
      });
  }  
  notifier.notify_one();
  event.wait();
}

}