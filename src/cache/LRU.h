/**
  ******************************************************************************
  * @file           : LFU.h
  * @author         : xy
  * @brief          : LRU cache
  * @attention      : None
  * @date           : 2025/3/26
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_LRU_H_
#define CACHE_SRC_CACHE_LRU_H_

#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "CachePolicy.h"

namespace Cache {

template<typename Key, typename Value>
class LRU;
template<typename Key, typename Value>
class LRUCache;

/**
 * @brief Node
 * @tparam Key
 * @tparam Value
 */
template<typename Key, typename Value>
class LruNode {
  friend class LRU<Key, Value>;
  friend class LRUCache<Key, Value>;
 public:
  LruNode(Key key, Value value)
	  : key_(key), value_(value), count_(0), prev_(nullptr), next_(nullptr) {}

  ~LruNode() {
	  prev_ = nullptr;
	  next_ = nullptr;
  }

 private:
  Key key_;
  Value value_;
  size_t count_;    // 缓存被访问次数
  std::shared_ptr<LruNode<Key, Value>> prev_;
  std::shared_ptr<LruNode<Key, Value>> next_;
};

/**
 * @brief LRU
 * @tparam Key
 * @tparam Value
 */
template<typename Key, typename Value>
class LRU : public CachePolicy<Key, Value> {
 public:
  using NodeType = LruNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeType>;
  using NodeMap = std::unordered_map<Key, NodePtr>;

  explicit LRU(size_t capacity = 1) : capacity_(capacity) { init(); }

  ~LRU() {
	  clearMap();
	  dummyHead_->next_ = nullptr;
	  dummyTail_->prev_ = nullptr;
  }

  NodeMap &nodeMap() { return nodeMap_; }

  void put(Key key, Value value) override {
	  // 如果存在，更新节点值, 并移到头部
	  auto it = nodeMap_.find(key);
	  if (it != nodeMap_.end()) {
		  it->second->value_ = value;
		  moveToHead(it->second);
		  return;
	  }
	  // 如果超过缓存容量，淘汰末尾节点
	  if (nodeMap_.size() >= capacity_) {
		  cacheLastNode();
	  }
	  // 头部插入节点
	  addHeadNode(key, value);
  }

  std::optional<Value> get(Key key) override {
	  auto it = nodeMap_.find(key);
	  if (it == nodeMap_.end()) {
		  return std::nullopt;
	  }
	  it->second->count_++;
	  // 节点存在，更新到头部
	  moveToHead(it->second);
	  // 返回节点值
	  return it->second->value_;
  }

  bool get(Key key, Value &value) override {
	  auto it = nodeMap_.find(key);
	  if (it == nodeMap_.end()) {
		  return false;
	  }
	  it->second->count_++;
	  moveToHead(it->second);
	  value = it->second->value_;
	  return true;
  }

 private:
  void init() {
	  dummyHead_ = std::make_shared<NodeType>(Key(), Value());
	  dummyTail_ = std::make_shared<NodeType>(Key(), Value());
	  dummyHead_->next_ = dummyTail_;
	  dummyTail_->prev_ = dummyHead_;
  }

  void clearMap() {
	  while (dummyHead_->next_ != dummyTail_) {
		  auto node = dummyHead_->next_;
		  removeNode(node);
		  nodeMap_.erase(node->key_);
	  }
	  nodeMap_.clear();
  }

  void moveToHead(NodePtr node) {
	  removeNode(node);
	  insertNode(node);
  }

  void removeNode(NodePtr node) {
	  node->prev_->next_ = node->next_;
	  node->next_->prev_ = node->prev_;
	  node->prev_ = nullptr;
	  node->next_ = nullptr;
  }

  void insertNode(NodePtr node) {
	  node->next_ = dummyHead_->next_;
	  dummyHead_->next_->prev_ = node;
	  dummyHead_->next_ = node;
	  node->prev_ = dummyHead_;
  }

  void cacheLastNode() {
	  auto node = dummyTail_->prev_;
	  nodeMap_.erase(node->key_);
	  removeNode(node);
  }

  void addHeadNode(Key key, Value value) {
	  auto newNode = std::make_shared<NodeType>(key, value);
	  insertNode(newNode);
	  nodeMap_[key] = newNode;
  }

 private:
  size_t capacity_;            // 缓存容量，超过容量触发淘汰机制
  NodePtr dummyHead_;        // 虚拟头结点
  NodePtr dummyTail_;        // 虚拟尾结点
  NodeMap nodeMap_;            // 目的：查询 key 的时间复杂度为 O(1)
};

/**
 * @brief K-LRU，让高频访问的缓存不容易失效，某个缓存，必须达到 k 次访问次数，才能加入到缓存中
 * @tparam Key
 * @tparam Value
 * @attention 内部维护一个 LRU, Key-Value 为 Key-Count，缓存访问次数（count） >= k 次，从 historyList_ 中移除，加入到缓存中
 */

template<typename Key, typename Value>
class KLru : public LRU<Key, Value> {
 public:
  explicit KLru(size_t capacity, size_t k)
	  : LRU<Key, Value>(capacity), k_(k) {}

  ~KLru() = default;

  std::optional<Value> get(Key key) {
	  int count = historyList_->get(key);
	  historyList_->put(key, ++count);
	  return LRU<Key, Value>::get(key);    // 调用父类的 get 方法
  }

  void put(Key key, Value value) {
	  // 缓存中存在，更新节点值
	  if (LRU<Key, Value>::get(key) != std::nullopt) {
		  LRU<Key, Value>::put(key, value);
	  }
	  int count = historyList_->get(key);
	  historyList_->put(key, ++count);
	  if (count >= k_) {
		  // 超过 k 次，从 historyList_ 中移除
		  historyList_->remove(key);
		  // 加入到缓存中
		  LRU<Key, Value>::put(key, value);
	  }
  }

 private:
  size_t k_;
  std::shared_ptr<LRU<Key, size_t>> historyList_;
};

template<typename Key, typename Value>
class MultiLRU {
  using LRUptr = std::shared_ptr<LRU<Key, Value>>;
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LruNode<Key, Value>>>;
 public:
  explicit MultiLRU(size_t capacity = 1)
	  : cache_(std::make_shared<LRU<Key, Value>>(capacity)), pending_(std::make_shared<LRU<Key, Value>>(capacity)) {}

  ~MultiLRU() = default;

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

#endif //CACHE_SRC_CACHE_LRU_H_
