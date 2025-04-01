/**
  ******************************************************************************
  * @file           : LFUCache.h
  * @author         : xy
  * @brief          : None
  * @attention      : None
  * @date           : 2025/3/28
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_LFUCACHE_H_
#define CACHE_SRC_CACHE_LFUCACHE_H_

#include <iostream>
#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <future>
#include <functional>

#include "LFU.h"

namespace Cache{

template<typename Key, typename Value>
class LFUThread {
  using Task = std::function<void()>;
  using LFUptr = std::shared_ptr<LFU<Key, Value>>;
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LfuNode<Key, Value>>>;
 public:
  LFUThread(size_t capacity, size_t id_)
	  : id_(id_), cache_(std::make_shared<MultiLFU<Key, Value>>(capacity)) {
	  startThread();
  }

  ~LFUThread() {
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
  PNodeMap &pending() {
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
  std::shared_ptr<MultiLFU<Key, Value>> cache_;
  std::queue<Task> tasks_;
  std::thread worker_;
  std::condition_variable condition_;
  std::mutex queueMutex_;
  std::atomic<bool> isStop_ = false;
};

template<typename Key, typename Value>
class LFUCache {
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LfuNode<Key, Value>>>;
 public:
  LFUCache(size_t capacity, size_t threadNum, size_t syncInterval = 3)
	  : index_(0), isSyncing_(true),
	  threadNum_(threadNum),
	  mainCache_(std::make_shared<MultiLFU<Key, Value>>(capacity)) {
	  for (size_t i = 0; i < threadNum_; ++i) {
		  threads_.push_back(std::make_shared<LFUThread<Key, Value>>(capacity, i));
	  }

	  // 开启定时同步
	  syncThread_ = std::thread([this, syncInterval]() {
		while (isSyncing_) {
			std::this_thread::sleep_for(std::chrono::seconds(syncInterval));
			syncCache();
		}
	  });
  }

  ~LFUCache() {
	  isSyncing_ = false;
	  if (syncThread_.joinable()) {
		  syncThread_.join();
	  }
  }

  void put(Key key, Value value, size_t index) {
	  checkIndex(index);
	  threads_[index]->commit([this, index, key, value]() {
		threads_[index]->put(key, value);
	  });
  }

  std::optional<Value> get(Key key, size_t index) {
	  checkIndex(index);
	  auto future = threads_[index]->commit([this, index, key]() {
		return threads_[index]->get(key);
	  });
	  return future.get();
  }

  void checkIndex(size_t &index) {
	  if (index >= threadNum_) {
		  index = selectThread();
	  }
  }

  void syncCache() {
	  // 防止特别同步时间太短，导致这边还在同步，就把容器交换，并发出现异常
	  std::lock_guard<std::mutex> lock_guard(mtx_);

	  // 收集所有子线程的 pending_ 缓存，合并到主缓存
	  for (const auto &subCache : threads_) {
		  const PNodeMap &pending = subCache->pending();
		  for (const auto &[key, node] : pending) {
			  mainCache_->put(node->key_, node->value_);
		  }
	  }
	  // 保证主缓存的一致性
	  const PNodeMap &cache = mainCache_->pending(false);
	  for (const auto &[key, node] : cache) {
		  std::cout << "syncCache key: " << node->key_ << " value: " << node->value_ << std::endl;
		  for (const auto &subCache : threads_) {
			  subCache->put(node->key_, node->value_);
		  }
	  }
  }

 private:

// 轮询算法
  size_t selectThread() {
	  return index_++ % threadNum_;
  }

 public:
// 删除拷贝语义
  LFUCache(const LFUCache &other) = delete;
  LFUCache &operator=(const LFUCache &other) = delete;
// 删除移动语义
  LFUCache(LFUCache &&other) = delete;
  LFUCache &operator=(LFUCache &&other) = delete;
 private:
  std::atomic<size_t> index_;
  const size_t threadNum_;
  std::mutex mtx_;
  std::vector<std::shared_ptr<LFUThread<Key, Value>>> threads_;
  std::thread syncThread_;
  std::atomic<bool> isSyncing_;
  std::shared_ptr<MultiLFU<Key, Value>> mainCache_;
};
}


#endif //CACHE_SRC_CACHE_LFUCACHE_H_
