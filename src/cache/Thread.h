/**
  ******************************************************************************
  * @file           : Thread.h
  * @author         : xy
  * @brief          : one thread one cache
  * @attention      : None
  * @date           : 2025/3/27
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_THREAD_H_
#define CACHE_SRC_CACHE_THREAD_H_

#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <future>
#include <functional>
#include "MultiLRU.h"

namespace Cache {
template<typename Key, typename Value>
class Thread {
  using Task = std::function<void()>;
  using LRUptr = std::shared_ptr<LRU<Key, Value>>;
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LruNode<Key, Value>>>;
 public:
  Thread(size_t capacity, size_t id_)
	  : id_(id_), cache_(std::make_shared<MultiLRU<Key, Value>>(capacity)) {
	  startThread();
  }

  ~Thread() {
	  // std::cout << "Thread id: " << id_ << " destructor" << std::endl;
	  stopThread();
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

  void put(Key key, Value value) {
	  cache_->put(key, value);
  }

  std::optional<Value> get(Key key) {
	  return cache_->get(key);
  }

  bool get(Key key, Value &value) {
	  return cache_->get(key, value);
  }

  void Print() {
	  std::cout << "Thread id: " << id_ << std::endl;
	  cache_->Print();
  }

  PNodeMap& pending() {
	  return cache_->pending();
  }

 private:
  void startThread() {
	  worker_ = std::thread([this] {
		while (!isStop_.load()) {
			std::unique_lock<std::mutex> uq_lock(queueMutex_);
			condition_.wait(uq_lock, [this]() {
			  return !tasks_.empty() || isStop_.load();
			});
			if (!tasks_.empty()) {
				auto task = tasks_.front();
				tasks_.pop();
				uq_lock.unlock();
				task();
			}
		}
	  });
  }

  void stopThread() {
	  isStop_ = true;
	  condition_.notify_one();
	  if (worker_.joinable()) {
		  worker_.join();
	  }
  }

 private:
  size_t id_;
  std::shared_ptr<MultiLRU<Key, Value>> cache_;
  std::queue<Task> tasks_;
  std::thread worker_;
  std::condition_variable condition_;
  std::mutex queueMutex_;
  std::atomic<bool> isStop_ = false;
};
}
#endif //CACHE_SRC_CACHE_THREAD_H_
