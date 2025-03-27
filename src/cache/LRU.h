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

#include <memory>
#include <unordered_map>

#include "CachePolicy.h"

namespace Cache {

/**
 * @brief 节点定义
 * @tparam Key
 * @tparam Value
 */
template<typename Key, typename Value>
class LRU;
template<typename Key, typename Value>
class LRUCache;
template<typename Key, typename Value>
class LruNode {
  friend class LRU<Key, Value>;
  friend class LRUCache<Key, Value>;
 public:
  LruNode(Key key, Value value)
	  : key_(key),
		value_(value),
		prev_(nullptr),
		next_(nullptr) {}

  // Getters
  Key key() const { return key_; }

  Value value() const { return value_; }

  // Setters
  void setValue(const Value &value) { value_ = value; }

  ~LruNode() {
	  prev_ = nullptr;
	  next_ = nullptr;
  }

 private:
  Key key_;
  Value value_;
  std::shared_ptr<LruNode<Key, Value>> prev_;
  std::shared_ptr<LruNode<Key, Value>> next_;
};

template<typename Key, typename Value>
class LRU : public CachePolicy<Key, Value> {
 public:
  using NodeType = LruNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeType>;
  using NodeMap = std::unordered_map<Key, NodePtr>;

  explicit LRU(size_t capacity) : capacity_(capacity) { init(); }

  ~LRU() {
	  // std::cout << "LRU destructor" << std::endl;
	  clearMap();
	  dummyHead_->next_ = nullptr;
	  dummyTail_->prev_ = nullptr;
  }

  void Print() {
	  NodePtr cur = dummyHead_->next_;
	  while (cur != dummyTail_) {
		  std::cout << "key: " << cur->key() << " value: " << cur->value() << std::endl;
		  cur = cur->next_;
	  }
  }

  NodeMap &nodeMap() { return nodeMap_; }

  void put(Key key, Value value) override {
	  // 如果存在，更新节点值, 并移到头部
	  auto it = nodeMap_.find(key);
	  if (it != nodeMap_.end()) {
		  it->second->setValue(value);
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
	  // 节点存在，更新到头部
	  moveToHead(it->second);
	  // 返回节点值
	  return it->second->value();
  }

  bool get(Key key, Value &value) override {
	  auto it = nodeMap_.find(key);
	  if (it == nodeMap_.end()) {
		  return false;
	  }
	  moveToHead(it->second);
	  value = it->second->value();
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
		  nodeMap_.erase(node->key());
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
  }

  void insertNode(NodePtr node) {
	  node->next_ = dummyHead_->next_;
	  dummyHead_->next_->prev_ = node;
	  dummyHead_->next_ = node;
	  node->prev_ = dummyHead_;
  }

  void cacheLastNode() {
	  auto node = dummyTail_->prev_;
	  nodeMap_.erase(node->key());
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

}

#endif //CACHE_SRC_CACHE_LRU_H_
