
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

class WorkerThread
{
public:
  WorkerThread();
  
  void shutdown();
  void cancel();

  template <typename Func, typename R = std::decay_t<std::invoke_result_t<Func&&>>>
  auto add(Func&& func) -> std::future<R>
  {
    auto task = std::packaged_task<R()>(std::forward<Func>(func));
    auto ret  = task.get_future();

    {
      std::lock_guard<std::mutex> lockg{lock};
      container.emplace_back(std::move(task));
    }

    // let a waiting thread know there is an available job
    notifier.notify_one();
    return ret;
  }

  bool isThisThread() const 
  {
    return thread.get_id() == std::this_thread::get_id();
  }

private:
  using Entry = std::packaged_task<void()>;

  std::condition_variable notifier;
  std::thread             thread;
  std::mutex              lock;
  std::deque<Entry>       container;
  std::atomic_bool        quit = false;
};
} // namespace terra