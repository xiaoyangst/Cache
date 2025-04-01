/**
  ******************************************************************************
  * @file           : LFU.h
  * @author         : xy
  * @brief          : LFU
  * @attention      :
  * @date           : 2025/3/28
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_LFU_H_
#define CACHE_SRC_CACHE_LFU_H_

#include <memory>
#include <unordered_map>
#include <cmath>
#include <mutex>
#include <algorithm>

#include "CachePolicy.h"

namespace Cache {

template<typename Key, typename Value>
class LFU;
template<typename Key, typename Value>
class FreqList;
template<typename Key, typename Value>
class LFUCache;

template<typename Key, typename Value>
class LfuNode {
  friend class LFU<Key, Value>;
  friend class FreqList<Key, Value>;
  friend class LFUCache<Key, Value>;
 public:
  LfuNode(Key key, Value value)
	  : key_(key), value_(value), count_(1), prev_(nullptr), next_(nullptr) {}

  ~LfuNode() {
	  prev_ = nullptr;
	  next_ = nullptr;
  }

 private:
  Key key_;
  Value value_;
  size_t count_;    // 缓存被访问次数
  std::shared_ptr<LfuNode<Key, Value>> prev_;
  std::shared_ptr<LfuNode<Key, Value>> next_;
};

template<typename Key, typename Value>
class FreqList {
  using NodeType = LfuNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeType>;
 public:
  explicit FreqList(size_t n) : freq_(n) { init(); }

  ~FreqList() {
	  dummyHead_->next_ = nullptr;
	  dummyTail_->prev_ = nullptr;
  }

  bool isEmpty() const {
	  return dummyHead_->next_ == dummyTail_;
  }

  void insertNode(NodePtr node) {
	  if (node == nullptr ||
		  dummyHead_ == nullptr ||
		  dummyTail_ == nullptr) {
		  return;
	  } // 确保后续访问节的点存在

	  node->next_ = dummyHead_->next_;
	  dummyHead_->next_->prev_ = node;
	  dummyHead_->next_ = node;
	  node->prev_ = dummyHead_;
  }

  void removeNode(NodePtr node) {
	  if (node == nullptr ||
		  dummyHead_ == nullptr ||
		  dummyTail_ == nullptr ||
		  node->prev_ == nullptr ||
		  node->next_ == nullptr) {
		  return;
	  } // 确保后续访问节的点存在
	  node->prev_->next_ = node->next_;
	  node->next_->prev_ = node->prev_;
	  node->prev_ = nullptr;
	  node->next_ = nullptr;
  }

  NodePtr getFirstNode() const { return dummyHead_->next_; }

 private:
  void init() {
	  dummyHead_ = std::make_shared<NodeType>(Key(), Value());
	  dummyTail_ = std::make_shared<NodeType>(Key(), Value());
	  dummyHead_->next_ = dummyTail_;
	  dummyTail_->prev_ = dummyHead_;
  }

 private:
  size_t freq_;    // 频率
  NodePtr dummyHead_;
  NodePtr dummyTail_;
};

template<typename Key, typename Value>
class MultiLFU;

template<typename Key, typename Value>
class LFU : public CachePolicy<Key, Value> {
  friend class LFUCache<Key, Value>;
  friend class MultiLFU<Key, Value>;

  using NodeType = LfuNode<Key, Value>;
  using NodePtr = std::shared_ptr<NodeType>;
  using NodeMap = std::unordered_map<Key, NodePtr>;
  using FreqListMap = std::unordered_map<int, std::shared_ptr<FreqList<Key, Value>>>;
 public:
  explicit LFU(size_t capacity = 1, int maxAverageNum = 10)
	  : capacity_(capacity)
		, minFreq_(1)
		, maxAverageNum_(maxAverageNum)
		, curAverageNum_(0)
		, curTotalNum_(0) {

  }

  ~LFU() {
	  nodeMap_.clear();
	  freqToFreqList_.clear();
  }

  void put(Key key, Value value) override {
	  auto it = nodeMap_.find(key);
	  if (it != nodeMap_.end()) {
		  it->second->value_ = value;
		  updateNode(it->second);
		  return;
	  }
	  putNode(key, value);
  }

  std::optional<Value> get(Key key) override {
	  auto it = nodeMap_.find(key);
	  if (it != nodeMap_.end()) {
		  updateNode(it->second);
		  return it->second->value_;
	  }
	  return std::nullopt;
  }

 private:
  void putNode(Key key, Value value) {
	  // 判断缓存是否已满
	  if (nodeMap_.size() == capacity_) {
		  removeMinFreqNode();
	  }
	  NodePtr node = std::make_shared<NodeType>(key, value);
	  nodeMap_[key] = node;
	  addToFreqList(node);
	  addFreqNum();
	  minFreq_ = 1;
  }

  void updateNode(NodePtr node) {
	  // 频率更新，从当前频率链表中移除，并加入新的频率链表
	  removeFromFreqList(node);
	  node->count_++;
	  addToFreqList(node);

	  // 由于有从某个频率的链表中删除节点，可能导致链表为空
	  // 有可能导致最小频率的链表为空，需要更新最小频率
	  auto prevCount = node->count_ - 1;
	  if (prevCount == minFreq_ && freqToFreqList_[prevCount]->isEmpty()) {
		  minFreq_++;
	  }
  }

  void removeMinFreqNode() {
	  auto node = freqToFreqList_[minFreq_]->getFirstNode();
	  removeFromFreqList(node);
	  nodeMap_.erase(node->key_);
  }

  void removeFromFreqList(NodePtr node) {
	  auto count = node->count_;
	  freqToFreqList_[count]->removeNode(node);
  }

  void addToFreqList(NodePtr node) {
	  auto count = node->count_;
	  if (freqToFreqList_.find(count) == freqToFreqList_.end()) {
		  freqToFreqList_[count] = std::make_shared<FreqList<Key, Value>>(count);
	  }
	  freqToFreqList_[count]->insertNode(node);
  }

  void addFreqNum() {
	  ++curTotalNum_;
	  if (nodeMap_.empty()) {
		  curAverageNum_ = 0;
	  } else {
		  curAverageNum_ = curTotalNum_ / nodeMap_.size();
	  }
	  if (curAverageNum_ > maxAverageNum_) {
		  handleOverMaxAverageNum();
	  }
  }

  void handleOverMaxAverageNum() {
	  if (nodeMap_.empty()) { return; }

	  // 所有节点频率减半，重新加入到新的频率链表中
	  for (auto &nodeItm : nodeMap_) {
		  if (nodeItm.second == nullptr) { continue; }

		  auto node = nodeItm.second;

		  // 先移除
		  removeFromFreqList(node);

		  // 频率减半
		  node->count_ -= maxAverageNum_ / 2;
		  if (node->count_ < 1) { node->count_ = 1; }

		  // 更新之后，加入到频率链表中
		  addToFreqList(node);
	  }

	  // 更新最小频率
	  updateMinFreq();
  }

  void updateMinFreq() {
	  minFreq_ = INT8_MAX;
	  for (const auto &freqItm : freqToFreqList_) {
		  if (freqItm.second && !freqItm.second->isEmpty()) {
			  minFreq_ = std::min(minFreq_, freqItm.first);
		  }
	  }
	  if (minFreq_ == INT8_MAX) {
		  minFreq_ = 1;
	  }
  }

  NodeMap &nodeMap() { return nodeMap_; }

 private:
  size_t capacity_;        // 缓存容量
  int minFreq_;        // 最小访问频次
  size_t maxAverageNum_;    // 最大平均访问频次
  size_t curAverageNum_;    // 当前平均访问频次
  size_t curTotalNum_;    // 当前总访问频次
  NodeMap nodeMap_;
  FreqListMap freqToFreqList_;    // key 为访问频次，value 为对应的链表，记录着相同访问频次的节点
};

template<typename Key, typename Value>
class MultiLFU {
  using LFUptr = std::shared_ptr<LFU<Key, Value>>;
  using PNodeMap = std::unordered_map<Key, std::shared_ptr<LfuNode<Key, Value>>>;
 public:
  explicit MultiLFU(size_t capacity = 1)
	  : cache_(std::make_shared<LFU<Key, Value>>(capacity)), pending_(std::make_shared<LFU<Key, Value>>(capacity)) {}

  ~MultiLFU() = default;

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
  LFUptr cache_;           // 对外提供服务的缓存
  LFUptr pending_;        // 增量缓存
  std::mutex mutex_;
};

}

#endif //CACHE_SRC_CACHE_LFU_H_
