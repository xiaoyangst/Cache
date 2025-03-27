/**
  ******************************************************************************
  * @file           : LRUCache.h
  * @author         : xy
  * @brief          : LRU cache manager
  * @attention      : None
  * @date           : 2025/3/27
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_LRUCACHE_H_
#define CACHE_SRC_CACHE_LRUCACHE_H_

#include "Thread.h"

namespace Cache {
template<typename Key, typename Value>
class LRUCache {
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LruNode<Key, Value>>>;
 public:
  LRUCache(size_t capacity, size_t threadNum)
	  : index_(0), threadNum_(threadNum), mainCache_(std::make_shared<MultiLRU<Key, Value>>(capacity)) {
	  for (size_t i = 0; i < threadNum_; ++i) {
		  threads_.push_back(std::make_shared<Thread<Key, Value>>(capacity, i));
	  }
  }

  ~LRUCache() {
	  // std::cout << "LRUCache destructor" << std::endl;
  }

  void Print() {
	  for (auto &thread_ : threads_) {
		  thread_->Print();
	  }
  }

  void put(Key key, Value value) {
	  size_t index = selectThread();
	  threads_[index]->commit([this, index, key, value]() {
		threads_[index]->put(key, value);
	  });
  }

  std::optional<Value> get(Key key) {
	  size_t index = selectThread();
	  auto future = threads_[index]->commit([this, index, key]() {
		return threads_[index]->get(key);
	  });
	  return future.get();
  }

  bool get(Key key, Value &value) {
	  size_t index = selectThread();
	  auto future = threads_[index]->commit([this, index, key, &value]() {
		return threads_[index]->get(key, value);
	  });
	  return future.get();
  }

  void syncCache() {
	  // std::cout << "syncCache start" << std::endl;

	  // 收集所有子线程的 pending_ 缓存，合并到主缓存
	  for (const auto &subCache : threads_) {
		  const PNodeMap &pending = subCache->pending();
		  for (const auto &[key, node] : pending) {
			  mainCache_->put(node->key(), node->value());
		  }
	  }
	  // 清空所有子线程的 pending_ 缓存
	  // 保证主缓存的一致性
	  // std::cout << "syncCache middle" << std::endl;
	  const PNodeMap &cache = mainCache_->pending(false);
	  for (const auto &[key, node] : cache) {
		  // std::cout << "syncCache key: " << node->key() << " value: " << node->value() << std::endl;
		  for (const auto &subCache : threads_) {
			  subCache->put(node->key(), node->value());
		  }
	  }

	  // std::cout << "syncCache end" << std::endl;
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
  std::shared_ptr<MultiLRU<Key, Value>> mainCache_;
};
}
#endif //CACHE_SRC_CACHE_LRUCACHE_H_
