/*!
 * @creator eli 
 * @date 1/11/18
 */

#ifndef TRADING_SYSTEM_BARRIER_HPP
#define TRADING_SYSTEM_BARRIER_HPP

#include <mutex>
#include <condition_variable>

class barrier
{
public:
  explicit barrier(std::size_t count)
    : count_{count}
  { }

  void wait()
  {
    std::unique_lock<std::mutex> lock{mutex_};
    if (--count_ == 0) {
      cv_.notify_all();
    } else {
      cv_.wait(lock, [this] { return count_ == 0; });
    }
  }
private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::size_t count_;
};


#endif //TRADING_SYSTEM_BARRIER_HPP
