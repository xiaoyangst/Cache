/**
  ******************************************************************************
  * @file           : ArcLRU.h
  * @author         : xy
  * @brief          : None
  * @attention      : None
  * @date           : 2025/3/29
  ******************************************************************************
  */

#ifndef CACHE_SRC_ARCCACHE_ARCLRU_H_
#define CACHE_SRC_ARCCACHE_ARCLRU_H_

#include <unordered_map>
#include <mutex>
#include <optional>
#include "ArcNode.h"

namespace Cache {

template<typename Key, typename Value>
class ArcLRU {
  using NodeType = ArcNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeType>;
  using NodeMap = std::unordered_map<Key, NodePtr>;
 public:
  explicit ArcLRU(size_t capacity, size_t transformValue)
	  : capacity_(capacity)
		, eliminateCapacity_(capacity)
		, transformValue_(transformValue) {
	  init();
  }

  bool put(Key key, Value value) {
	  if (capacity_ == 0) return false;

	  auto it = mainCache_.find(key);
	  if (it != mainCache_.end()) {
		  return updateNode(it->second, value);
	  }
	  return addNode(key, value);
  }

  std::optional<Value> get(Key key, bool &shouldTransform) {
	  auto it = mainCache_.find(key);
	  if (it != mainCache_.end()) {
		  shouldTransform = updateNodeAccess(it->second);
		  return it->second->value_;
	  }
	  return std::nullopt;
  }

  void increaseCapacity() { ++capacity_; }

  bool decreaseCapacity() {
	  if (capacity_ <= 0) return false;
	  if (mainCache_.size() == capacity_) {
		  eliminateNode();
	  }
	  --capacity_;
	  return true;
  }

  // 检查是否在淘汰链表中
  bool checkEliminate(Key key) {
	  auto it = eliminateCache_.find(key);
	  if (it != eliminateCache_.end()) {
		  return true;
	  }
	  return false;
  }

  // 从淘汰链表中移除
  Value delEliminateNode(Key key) {
	  auto it = eliminateCache_.find(key);
	  Value val;
	  if (it != eliminateCache_.end()) {
		  val = it->second->value_;
		  removeNode(it->second);
		  eliminateCache_.erase(it);
	  }
	  return val;
  }



 private:
  bool updateNode(NodePtr node, const Value &value) {
	  if (node == nullptr) return false;

	  node->value_ = value;
	  moveToFront(node);
	  addToFront(node);
	  return true;
  }

  bool addNode(Key key, const Value &value) {
	  if (mainCache_.size() == capacity_) {
		  eliminateNode();
	  }
	  auto node = std::make_shared<NodeType>(key, value);
	  mainCache_[key] = node;
	  addToFront(node);
	  return true;
  }

  void moveToFront(NodePtr node) {
	  if (node == nullptr ||
		  node->prev_ == nullptr ||
		  node->next_ == nullptr) {
		  return;
	  }

	  node->prev_->next_ = node->next_;
	  node->next_->prev_ = node->prev_;
  }

  void addToFront(NodePtr node) {
	  if (node == nullptr ||
		  node->prev_ != nullptr ||
		  node->next_ != nullptr ||
		  mainHead_ == nullptr) {
		  return;
	  }

	  node->next_ = mainHead_->next_;
	  node->prev_ = mainHead_;
	  mainHead_->next_->prev_ = node;
	  mainHead_->next_ = node;
  }

  // 淘汰的缓存节点要加入到淘汰链表中
  void eliminateNode() {
	  // 从最后获取一个有效节点，同时确保不是头尾虚拟节点这种无效节点
	  auto leastRecent = mainTail_->prev_;
	  if (leastRecent == mainHead_) return;

	  // 从主缓存中移除
	  removeNode(leastRecent);
	  mainCache_.erase(leastRecent->key_);

	  // 加入到淘汰缓存，确保容量足够
	  if (eliminateCache_.size() == eliminateCapacity_) {
		  removeFromEliminate();
	  }
	  addToEliminate(leastRecent);
	  eliminateCache_[leastRecent->key_] = leastRecent;
  }

  void removeNode(NodePtr node) {
	  if (node == nullptr ||
		  node->prev_ == nullptr ||
		  node->next_ == nullptr) {
		  return;
	  }

	  node->prev_->next_ = node->next_;
	  node->next_->prev_ = node->prev_;
  }

  void addToEliminate(NodePtr node) {
	  if (node == nullptr ||
		  node->prev_ == nullptr ||
		  node->next_ == nullptr ||
		  eliminateHead_ == nullptr) {
		  return;
	  }

	  node->next_ = eliminateHead_->next_;
	  node->prev_ = eliminateHead_;
	  eliminateHead_->next_->prev_ = node;
	  eliminateHead_->next_ = node;
  }

  void removeFromEliminate() {
	  auto leastRecent = eliminateTail_->prev_;
	  if (leastRecent == eliminateHead_) return;
	  removeNode(leastRecent);
	  eliminateCache_.erase(leastRecent->key_);
  }

  bool updateNodeAccess(NodePtr node) {
	  moveToFront(node);
	  node->count_ = 0;    // 重置节点的访问计数
	  return node->count_ >= transformValue_;
  }

  void init() {
	  mainHead_ = std::make_shared<NodeType>();
	  mainTail_ = std::make_shared<NodeType>();
	  mainHead_->next_ = mainTail_;
	  mainTail_->prev_ = mainHead_;

	  eliminateHead_ = std::make_shared<NodeType>();
	  eliminateTail_ = std::make_shared<NodeType>();
	  eliminateHead_->next_ = eliminateTail_;
	  eliminateTail_->prev_ = eliminateHead_;
  }

 private:
  size_t capacity_;                // 存储缓存的容量
  size_t eliminateCapacity_;    // 存储淘汰缓存的容量
  size_t transformValue_;        // 切换 LFU 或 LRU 的门槛值
  std::mutex mutex_;            // 互斥锁

  NodeMap mainCache_;            // 主缓存
  NodeMap eliminateCache_;    // 淘汰缓存

  NodePtr mainHead_;            // 主缓存头节点
  NodePtr mainTail_;            // 主缓存尾节点

  NodePtr eliminateHead_;        // 淘汰缓存头节点
  NodePtr eliminateTail_;        // 淘汰缓存尾节点;
};

}

#endif //CACHE_SRC_ARCCACHE_ARCLRU_H_
