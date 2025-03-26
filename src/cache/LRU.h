/**
  ******************************************************************************
  * @file           : LFU.h
  * @author         : xy
  * @brief          : LRU
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
class LruNode {
  friend class LRU<Key, Value>;
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

 private:
  Key key_;
  Value value_;
  std::shared_ptr<LruNode<Key, Value>> prev_;
  std::shared_ptr<LruNode<Key, Value>> next_;
};

template<typename Key, typename Value>
class LRU : CachePolicy<Key, Value> {
 public:
  using NodeTyp = LruNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeTyp>;
  using NodeMap = std::unordered_map<Key, NodePtr>;

  explicit LRU(int capacity) : capacity_(capacity) { init(); }

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
	  addHeadNode(key,value);
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
	  dummyHead_ = std::make_shared<NodeTyp>(Key(), Value());
	  dummyTail_ = std::make_shared<NodeTyp>(Key(), Value());
	  dummyHead_->next_ = dummyTail_;
	  dummyTail_->prev_ = dummyHead_;
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
	  auto tailPrev = dummyTail_->prev_;
	  node->prev_ = tailPrev;
	  node->next_ = dummyTail_;
	  dummyTail_->prev_ = node;
  }

  void cacheLastNode(){
	  auto node = dummyTail_->prev_;
	  removeNode(node);
	  nodeMap_.erase(node->key());
  }

  void addHeadNode(Key key,Value value){
	  auto newNode = std::make_shared<NodeTyp>(key, value);
	  insertNode(newNode);
	  nodeMap_[key] = newNode;
  }

 private:
  int capacity_;        // 缓存容量，超过容量触发淘汰机制
  NodePtr dummyHead_;    // 虚拟头结点
  NodePtr dummyTail_;    // 虚拟尾结点
  NodeMap nodeMap_;        // 目的：查询 key 的时间复杂度为 O(1)
};

}

#endif //CACHE_SRC_CACHE_LRU_H_
