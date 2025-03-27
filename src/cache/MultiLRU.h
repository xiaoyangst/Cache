/**
  ******************************************************************************
  * @file           : MultiLRU.h
  * @author         : xy
  * @brief          : double cache
  * @attention      : None
  * @date           : 2025/3/27
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_MULTILRU_H_
#define CACHE_SRC_CACHE_MULTILRU_H_

#include <iostream>
#include <mutex>
#include "LRU.h"
namespace Cache {
template<typename Key, typename Value>
class MultiLRU {
  using LRUptr = std::shared_ptr<LRU<Key, Value>>;
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LruNode<Key, Value>>>;
 public:
  explicit MultiLRU(size_t capacity)
	  : cache_(std::make_shared<LRU<Key, Value>>(capacity)),
		pending_(std::make_shared<LRU<Key, Value>>(capacity)) {}

  ~MultiLRU() {
	  // std::cout << "MultiLRU destructor" << std::endl;
  }

  void Print() {
	  std::cout << "cache_:" << std::endl;
	  cache_->Print();
	  std::cout << "================================" << std::endl;
	  std::cout << "pending_:" << std::endl;
	  pending_->Print();
	  std::cout << "================================" << std::endl;
  }

  void put(Key key, Value value) {
	  cache_->put(key, value);
	  pending_->put(key, value);
  }

  std::optional<Value> get(Key key) {
	  return cache_->get(key);
  }

  bool get(Key key, Value &value) {
	  return cache_->get(key, value);
  }

  PNodeMap &pending(bool isSwap = true) {
	  if (isSwap) {
		  swap();
	  }
	  return pending_->nodeMap();
  }

  void clearPending() {
  }

  void swap() {        // 得加锁
	  std::lock_guard<std::mutex> _lock(mutex_);
	  std::swap(cache_, pending_);
  }

 private:
  LRUptr cache_;           // 对外提供服务的缓存
  LRUptr pending_;        // 增量缓存
  std::mutex mutex_;
};
}
#endif //CACHE_SRC_CACHE_MULTILRU_H_
