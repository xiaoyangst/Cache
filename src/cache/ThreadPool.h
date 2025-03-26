/**
  ******************************************************************************
  * @file           : ThreadPool.h
  * @author         : xy
  * @brief          : 线程池
  * @attention      : None
  * @date           : 2025/3/25
  ******************************************************************************
  */

#ifndef KVSTORE_SRC_UTILS_THREADPOOL_H_
#define KVSTORE_SRC_UTILS_THREADPOOL_H_

#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <atomic>
#include <vector>
#include <future>

class ThreadPool {
  using Task = std::function<void()>;
 public:
  ~ThreadPool() {
	  stop();
  }

  static ThreadPool &getInstance() {        // C++11 保证线程安全
	  static ThreadPool thread_pool;
	  return thread_pool;
  }

  template<class F, class... Args>
  void push(F &&f, Args &&... args) {
	  using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
	  auto task = std::make_shared<std::packaged_task<RetType()>>(
		  std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	  if (isStop_.load()) return;
	  {
		  std::lock_guard<std::mutex> lock(queueMutex_);
		  tasks_.emplace([task]() {
			(*task)();
		  });
	  }
	  condition_.notify_one();
  }

  template<class F, class... Args>
  auto commit(F &&f, Args &&... args) ->
  std::future<decltype(std::forward<F>(f)(std::forward<Args>(args)...))> {
	  using RetType = decltype(std::forward<F>(f)(std::forward<Args>(args)...));
	  if (isStop_.load()) {
		  return std::future<RetType>{};
	  }

	  auto task = std::make_shared<std::packaged_task<RetType()>>(
		  std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	  std::future<RetType> ret = task->get_future();
	  {
		  std::lock_guard<std::mutex> lock(queueMutex_);
		  tasks_.emplace([task] { (*task)(); });
	  }
	  condition_.notify_one();
	  return ret;
  }

  int curThreadsNum() {
	  return numThreads_.load();
  }

  // 不可拷贝
  ThreadPool(const ThreadPool &other) = delete;
  ThreadPool &operator=(const ThreadPool &other) = delete;
 private:
  explicit ThreadPool(int numThreads = std::thread::hardware_concurrency()) {
	  if (numThreads_ <= 1) {
		  numThreads_ = 2;
	  } else {
		  numThreads_ = numThreads;
	  }

	  start();
  }

  void start() {
	  for (int i = 0; i < numThreads_; ++i) {
		  workerThreads_.emplace_back([this] {
			while (!isStop_.load()) {
				std::unique_lock<std::mutex> uq_lock(queueMutex_);
				condition_.wait(uq_lock, [this]() {
				  return !tasks_.empty() || isStop_.load();
				});
				if (!tasks_.empty()) {
					auto task = tasks_.front();
					tasks_.pop();
					uq_lock.unlock();
					--numThreads_;
					task();
					++numThreads_;
				}
			}
		  });
	  }
  }

  void stop() {
	  isStop_.store(true);
	  condition_.notify_all();
	  for (auto &worker : workerThreads_) {
		  if (worker.joinable()) {
			  worker.join();
		  }
	  }
  }

 private:
  std::vector<std::thread> workerThreads_;
  std::queue<Task> tasks_;
  std::condition_variable condition_;
  std::mutex queueMutex_;
  std::atomic<bool> isStop_ = false;
  std::atomic<int> numThreads_ = 0;
};

#endif //KVSTORE_SRC_UTILS_THREADPOOL_H_
