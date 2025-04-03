/**
  ******************************************************************************
  * @file           : ArcLFU.h
  * @author         : xy
  * @brief          : None
  * @attention      : None
  * @date           : 2025/3/29
  ******************************************************************************
  */

#ifndef CACHE_SRC_ARCCACHE_ARCLFU_H_
#define CACHE_SRC_ARCCACHE_ARCLFU_H_

#include <unordered_map>
#include <list>

#include "ArcNode.h"

namespace Cache {
template<typename Key, typename Value>
class ArcLFU {
  using NodeType = ArcNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeType>;
  using NodeMap = std::unordered_map<Key, NodePtr>;
  using FreqMap = std::unordered_map<size_t, std::list<NodePtr>>;
 public:
  explicit ArcLFU(size_t capacity, size_t transformValue)
	  : capacity_(capacity)
		, eliminateCapacity_(capacity)
		, transformValue_(transformValue)
		, minFreq_(0) {
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

  std::optional<Value> get(Key key) {
	  auto it = mainCache_.find(key);
	  if (it != mainCache_.end()) {
		  updateNodeFrequency(it->second);
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
  void delEliminateNode(Key key) {
	  auto it = eliminateCache_.find(key);
	  if (it != eliminateCache_.end()) {
		  removeNode(it->second);
		  eliminateCache_.erase(it);
	  }
  }

 private:
  void init() {
	  eliminateHead_ = std::make_shared<NodeType>();
	  eliminateTail_ = std::make_shared<NodeType>();
	  eliminateHead_->next_ = eliminateTail_;
	  eliminateTail_->prev_ = eliminateHead_;
  }

  bool updateNode(NodePtr node, Value &value) {
	  if (node == nullptr) return false;

	  node->value_ = value;
	  updateNodeFrequency(node);
	  return true;
  }

  void updateNodeFrequency(NodePtr node) {
	  auto oldFreq = node->count_;
	  node->count_++;
	  auto newFreq = node->count_;

	  // 从原旧频率链表删除
	  auto &oldList = freqMap_[oldFreq];
	  oldList.remove(node);
	  // 可能当前频率链表为空了，如果如此，删除该频率项，并可能更新最小频率
	  if (oldList.empty()) {
		  freqMap_.erase(oldFreq);
		  if (oldFreq == minFreq_) {
			  minFreq_ = newFreq;
		  }
	  }

	  // 添加到新频率链表
	  if (freqMap_.find(newFreq) == freqMap_.end()) {
		  freqMap_[newFreq] = std::list<NodePtr>();
	  }
	  freqMap_[newFreq].push_back(node);
  }

  bool addNode(Key key, Value value) {
	  if (mainCache_.size() >= capacity_) {
		  eliminateNode();
	  }
	  auto node = std::make_shared<NodeType>(key, value);
	  mainCache_[key] = node;

	  if (freqMap_.find(1) == freqMap_.end()) {  // 确保频率为 1 的链表存在
		  freqMap_[1] = std::list<NodePtr>();
	  }
	  freqMap_[1].push_back(node);
	  minFreq_ = 1;

	  return true;
  }

  void eliminateNode() {
	  if (freqMap_.empty()) {
		  return;
	  }

	  // 获取最小频率链表
	  auto &minFreqList = freqMap_[minFreq_];
	  if (minFreqList.empty()) {
		  return;
	  }

	  // 从最小频率链表中删除
	  auto leastRecent = minFreqList.front();
	  minFreqList.pop_front();

	  // 如果该链表为空，则删除该频率项
	  if (minFreqList.empty()) {
		  freqMap_.erase(minFreq_);
		  if (!freqMap_.empty()) {
			  minFreq_ = freqMap_.begin()->first;
		  }
	  }

	  // 既然淘汰一个节点了，那么就要将其添加到淘汰链表中
	  if (eliminateCache_.size() >= eliminateCapacity_) {
		  removeFromEliminate();
	  }
	  addNodeToEliminate(leastRecent);

	  mainCache_.erase(leastRecent->key_);
  }

  void removeFromEliminate() {
	  auto eliminateLastNode = eliminateTail_->prev_;
	  if (eliminateLastNode != eliminateTail_) {
		  removeNode(eliminateLastNode);
		  eliminateCache_.erase(eliminateLastNode->key_);
	  }
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

  void addNodeToEliminate(NodePtr node) {
	  if (node == nullptr ||
		  node->prev_ == nullptr ||
		  node->next_ == nullptr ||
		  eliminateHead_ == nullptr) {
		  return;
	  }

	  node->next_ = eliminateTail_;
	  node->prev_ = eliminateTail_->prev_;
	  eliminateTail_->prev_->next_ = node;
	  eliminateTail_->prev_ = node;
	  eliminateCache_[node->key_] = node;
  }

 private:
  size_t capacity_;
  size_t eliminateCapacity_;
  size_t transformValue_;
  int minFreq_;

  NodeMap mainCache_;
  NodeMap eliminateCache_;
  FreqMap freqMap_;

  NodePtr eliminateHead_;
  NodePtr eliminateTail_;
};
}

#endif //CACHE_SRC_ARCCACHE_ARCLFU_H_
