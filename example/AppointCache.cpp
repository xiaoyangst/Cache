#include <iostream>
#include <cassert>
#include "LRUCache.h"
#include "LFUCache.h"

using namespace Cache;

void testLRU() {
	LRUCache<std::string, int> cache(10, 2);

	// 在第一个线程中添加数据
	cache.put("one", 1, 0);
	// 在第二个线程中添加数据
	cache.put("two", 2, 1);

	// 在第一个线程中获取数据
	std::optional<int> result1 = cache.get("one", 0);
	assert(result1 != std::nullopt);
	assert(result1.value() == 1);

	std::optional<int> result2 = cache.get("two", 0);
	assert(result2 == std::nullopt);	// 此时第二个线程中添加的数据还未同步到第一个线程中，第一个线程肯定拿不到数据

	// 在第二个线程中获取数据
	std::optional<int> result3 = cache.get("two", 1);
	assert(result3 != std::nullopt);
	assert(result3.value() == 2);

	std::optional<int> result4 = cache.get("one", 1);
	assert(result4 == std::nullopt);	// 此时第一个线程中添加的数据还未同步到第二个线程中，第二个线程肯定拿不到数据

	std::this_thread::sleep_for(std::chrono::seconds(10));

	// 经过两次同步之后，第一个线程中可以获取到第二个线程中添加的数据，第二个线程同理

	std::optional<int> result5 = cache.get("one", 0);
	assert(result5 != std::nullopt);
	assert(result5.value() == 1);

	std::optional<int> result6 = cache.get("two", 0);
	assert(result6 != std::nullopt);
	assert(result6.value() == 2);

	std::optional<int> result7 = cache.get("two", 1);
	assert(result7 != std::nullopt);
	assert(result7.value() == 2);

	std::optional<int> result8 = cache.get("one", 1);
	assert(result8 != std::nullopt);
	assert(result8.value() == 1);

}

void testLFU(){
	LFUCache<std::string, int> cache(10, 2);

	// 在第一个线程中添加数据
	cache.put("one", 1, 0);
	// 在第二个线程中添加数据
	cache.put("two", 2, 1);

	// 在第一个线程中获取数据
	std::optional<int> result1 = cache.get("one", 0);
	assert(result1 != std::nullopt);
	assert(result1.value() == 1);

	std::optional<int> result2 = cache.get("two", 0);
	assert(result2 == std::nullopt);	// 此时第二个线程中添加的数据还未同步到第一个线程中，第一个线程肯定拿不到数据

	// 在第二个线程中获取数据
	std::optional<int> result3 = cache.get("two", 1);
	assert(result3 != std::nullopt);
	assert(result3.value() == 2);

	std::optional<int> result4 = cache.get("one", 1);
	assert(result4 == std::nullopt);	// 此时第一个线程中添加的数据还未同步到第二个线程中，第二个线程肯定拿不到数据

	std::this_thread::sleep_for(std::chrono::seconds(10));

	// 经过两次同步之后，第一个线程中可以获取到第二个线程中添加的数据，第二个线程同理

	std::optional<int> result5 = cache.get("one", 0);
	assert(result5 != std::nullopt);
	assert(result5.value() == 1);

	std::optional<int> result6 = cache.get("two", 0);
	assert(result6 != std::nullopt);
	assert(result6.value() == 2);

	std::optional<int> result7 = cache.get("two", 1);
	assert(result7 != std::nullopt);
	assert(result7.value() == 2);

	std::optional<int> result8 = cache.get("one", 1);
	assert(result8 != std::nullopt);
	assert(result8.value() == 1);
}

using namespace std;

int main() {

	// testLRU();

	 testLFU();

	return 0;
}