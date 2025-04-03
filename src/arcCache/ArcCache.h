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

#include "CachePolicy.h"
#include "ArcLFU.h"
#include "ArcLRU.h"

namespace Cache{
template<typename Key, typename Value>
class ArcCache : public CachePolicy<Key,Value>{
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
  }

  std::optional<Value> get(Key key) {

	  if (arcLRU->checkEliminate(key)) {    // 如果在 LRU 淘汰链表
		  if (arcLFU->decreaseCapacity()) {
			  arcLRU->increaseCapacity();
		  }
		  arcLRU->delEliminateNode(key);
	  } else if (arcLFU->checkEliminate(key)) {    // 如果在 LFU 淘汰链表
		  if (arcLRU->decreaseCapacity()) {
			  arcLFU->increaseCapacity();
		  }
		  arcLFU->delEliminateNode(key);
	  }

	  auto shouldTransform = false;
	  auto re = arcLRU->get(key, shouldTransform);
	  if (re != std::nullopt) {
		  if (shouldTransform) {
			  arcLFU->put(key, re.value());
		  }
	  } else {
		  re = arcLFU->get(key);
	  }
	  return re;
  }

 private:
  size_t capacity_;
  size_t transformThreshold_;
  std::unique_ptr<ArcLRU<Key, Value>> arcLRU;
  std::unique_ptr<ArcLFU<Key, Value>> arcLFU;
};
}

#endif //CACHE_SRC_ARCCACHE_ARCCACHE_H_
