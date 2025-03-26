/**
  ******************************************************************************
  * @file           : CachePolicy.h
  * @author         : xy
  * @brief          : 缓存抽象类
  * @attention      : None
  * @date           : 2025/3/26
  ******************************************************************************
  */

#ifndef CACHE_SRC_CACHE_CACHEPOLICY_H_
#define CACHE_SRC_CACHE_CACHEPOLICY_H_

#include <optional>

namespace Cache {
template<typename Key, typename Value>
class CachePolicy {
 public:
  virtual ~CachePolicy() = default;
  virtual std::optional<Value> get(Key key) = 0;
  virtual bool get(Key key, Value &value) = 0;
  virtual void put(Key key, Value value) = 0;
};
}

#endif //CACHE_SRC_CACHE_CACHEPOLICY_H_
