/**
  ******************************************************************************
  * @file           : ArcCache.h
  * @author         : xy
  * @brief          : 智能选择 LRU 和 LFU 算法的缓存
  * @attention      : None
  * @date           : 2025/3/31
  ******************************************************************************
  */

#ifndef CACHE_SRC_ARCCACHE_ARCCACHE_H_
#define CACHE_SRC_ARCCACHE_ARCCACHE_H_

#include "ArcLFU.h"
#include "ArcLRU.h"

namespace Cache {
template<typename Key, typename Value>
class ArcCache {
 public:
  explicit ArcCache(size_t capacity, size_t transformThreshold)
	  : capacity_(capacity)
		, transformThreshold_(transformThreshold)
		, arcLRU(std::make_unique<ArcLRU<Key, Value>>(capacity, transformThreshold))
		, arcLFU(std::make_unique<ArcLFU<Key, Value>>
					 (capacity, transformThreshold)) {

  }

  void put(Key key, Value value) {
	  if (arcLRU->checkEliminate(key)) {
		  if (arcLFU->decreaseCapacity()) {
			  arcLRU->increaseCapacity();
		  }
		  arcLRU->delEliminateNode(key);
		  arcLFU->put(key, value);
	  } else if (arcLFU->checkEliminate(key)) {
		  if (arcLRU->decreaseCapacity()) {
			  arcLFU->increaseCapacity();
		  }
		  arcLFU->delEliminateNode(key);
		  arcLFU->put(key, value);
	  } else {
		  arcLRU->put(key, value);
	  }


	  /*
	  auto isEliminate = checkEliminateCaches(key);
	  if (isEliminate) {    // 在淘汰链表中
		  arcLRU->put(key, value);
	  } else {                // 不在淘汰链表中
		  if (arcLRU->put(key, value)) {
			  arcLFU->put(key, value);
		  }
	  }
	   */
  }

  std::optional<Value> get(Key key) {
	  auto lru = arcLRU->get(key);
	  if (lru != std::nullopt) {
		  return lru;
	  }
	  auto lfu = arcLFU->get(key);
	  if (lfu != std::nullopt) {
		  return lfu;
	  }

	  // 如果在 LRU 淘汰链表
	  if (arcLRU->checkEliminate(key)) {
		  if (arcLFU->decreaseCapacity()) {
			  arcLRU->increaseCapacity();
		  }
		  auto value = arcLRU->delEliminateNode(key);
		  arcLFU->put(key, value);
		  return std::nullopt;
	  }

	  // 如果在 LFU 淘汰链表
	  if (arcLFU->checkEliminate(key)) {
		  if (arcLRU->decreaseCapacity()) {
			  arcLFU->increaseCapacity();
		  }
		  auto value = arcLFU->delEliminateNode(key);
		  arcLFU->put(key, value);
		  return std::nullopt;
	  }

	  return std::nullopt;

	  /*
	  checkEliminateCaches(key);
	  auto shouldTransform = false;
	  if (arcLRU->get(key, shouldTransform) != std::nullopt) {
		  if (shouldTransform) {
			  arcLFU->put(key, arcLRU->get(key).value());
		  }
		  return arcLRU->get(key);
	  } else {
		  return arcLFU->get(key);
	  }
	   */
  }

 private:
  // 检查一个 Key 是否存在于 "Eliminate cache" 中
  // 并根据访问情况动态调整 LRU（T1）和 LFU（T2）的容量
  bool checkEliminateCaches(Key key) {
	  auto inEliminate = false;
	  if (arcLRU->checkEliminate(key)) {
		  // 说明最近访问的节点（LRU）更重要
		  // 应该尝试扩容 LRU，缩容 LFU
		  if (arcLFU->decreaseCapacity()) {
			  arcLRU->increaseCapacity();
		  }
		  inEliminate = true;
	  } else if (arcLFU->checkEliminate(key)) {
		  // 说明最近访问的节点（LFU）更重要
		  // 应该尝试扩容 LFU，缩容 LRU
		  if (arcLRU->decreaseCapacity()) {
			  arcLFU->increaseCapacity();
		  }
		  inEliminate = true;
	  }

	  return inEliminate;
  }

 private:
  size_t capacity_;
  size_t transformThreshold_;
  std::unique_ptr<ArcLRU<Key, Value>> arcLRU;
  std::unique_ptr<ArcLFU<Key, Value>> arcLFU;
};
}

#endif //CACHE_SRC_ARCCACHE_ARCCACHE_H_
