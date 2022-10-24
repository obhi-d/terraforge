
#include "WorkerThread.h"

namespace terra
{

WorkerThread::WorkerThread() 
{
  thread = std::thread([this]() 
    {
      while (!quit.load())
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
        
        container.front()();
        container.pop_front();
      }
    });
}

void WorkerThread::shutdown()
{
  quit = true;
  add([]() {}).wait();
  if (thread.joinable())
    thread.join();
}

void WorkerThread::cancel() 
{
  {
    std::unique_lock<std::mutex> lockg{lock};
    container.clear();
  }
  add([]() {}).wait();
}

}
