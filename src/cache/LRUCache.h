/**
  ******************************************************************************
  * @file           : LRUCache.h
  * @author         : xy
  * @brief          : LRU cache manager
  * @attention      : get 等方法提供 index 是为了让用户指定使用哪个缓存
  * @date           : 2025/3/27
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_LRUCACHE_H_
#define CACHE_SRC_CACHE_LRUCACHE_H_

#include <thread>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <future>
#include <functional>

#include "LRU.h"

namespace Cache {

/**
 * @brief 一个线程一个多缓存
 * @tparam Key
 * @tparam Value
 */
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
  std::shared_ptr<MultiLRU<Key, Value>> cache_;
  std::queue<Task> tasks_;
  std::thread worker_;
  std::condition_variable condition_;
  std::mutex queueMutex_;
  std::atomic<bool> isStop_ = false;
};

/**
 * @brief 多线程多缓存，可谓是多缓存的管理者
 * @tparam Key
 * @tparam Value
 */
template<typename Key, typename Value>
class LRUCache {
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LruNode<Key, Value>>>;
 public:
  LRUCache(size_t capacity, size_t threadNum, size_t syncInterval = 3)
	  : index_(0), isSyncing_(true),
		threadNum_(threadNum),
		mainCache_(std::make_shared<MultiLRU<Key, Value>>(capacity)) {
	  for (size_t i = 0; i < threadNum_; ++i) {
		  threads_.push_back(std::make_shared<Thread<Key, Value>>(capacity, i));
	  }

	  // 开启定时同步
	  syncThread_ = std::thread([this, syncInterval]() {
		while (isSyncing_) {
			std::this_thread::sleep_for(std::chrono::seconds(syncInterval));
			syncCache();
		}
	  });
  }

  ~LRUCache() {
	  isSyncing_ = false;
	  if (syncThread_.joinable()) {
		  syncThread_.join();
	  }
  }

  void Print() {
	  for (auto &thread_ : threads_) {
		  thread_->Print();
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

  bool get(Key key, Value &value, size_t index) {
	  checkIndex(index);
	  auto future = threads_[index]->commit([this, index, key, &value]() {
		return threads_[index]->get(key, value);
	  });
	  return future.get();
  }

  void checkIndex(size_t &index) {
	  if (index >= threadNum_) {
		  index = selectThread();
	  }
  }

  void syncCache() {
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
  LRUCache(const LRUCache &other) = delete;
  LRUCache &operator=(const LRUCache &other) = delete;
// 删除移动语义
  LRUCache(LRUCache &&other) = delete;
  LRUCache &operator=(LRUCache &&other) = delete;
 private:
  std::atomic<size_t> index_;
  const size_t threadNum_;
  std::vector<std::shared_ptr<Thread<Key, Value>>> threads_;
  std::thread syncThread_;
  std::atomic<bool> isSyncing_;
  std::shared_ptr<MultiLRU<Key, Value>> mainCache_;
};
}
#endif //CACHE_SRC_CACHE_LRUCACHE_H_
