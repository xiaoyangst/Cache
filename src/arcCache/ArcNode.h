/**
  ******************************************************************************
  * @file           : ArcNode.h
  * @author         : xy
  * @brief          : None
  * @attention      : None
  * @date           : 2025/3/29
  ******************************************************************************
  */

#ifndef CACHE_SRC_ARCCACHE_ARCNODE_H_
#define CACHE_SRC_ARCCACHE_ARCNODE_H_

#include <memory>

namespace Cache {
template<typename Key, typename Value>
class ArcNode {
 private:
  Key key_;
  Value value_;
  size_t count_;
  std::shared_ptr<ArcNode> prev_;
  std::shared_ptr<ArcNode> next_;

 public:
  ~ArcNode() {
	  prev_ = nullptr;
	  next_ = nullptr;
  }

  ArcNode(Key key, Value value)
	  : key_(key)
		, value_(value)
		, count_(1)
		, prev_(nullptr)
		, next_(nullptr) {}

  template<typename K, typename V> friend
  class ArcLRU;
  template<typename K, typename V> friend
  class ArcLFU;
};
}

#endif //CACHE_SRC_ARCCACHE_ARCNODE_H_
