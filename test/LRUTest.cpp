#include <iostream>
#include <chrono>
#include <random>
#include "LRU.h"

// 性能测试函数
void performanceTest(int capacity, int insertCount, int queryCount) {
	Cache::LRU<int, int> lru(capacity);

	// 随机数生成器
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(1, 1000000);

	// 插入操作性能测试
	auto startInsert = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < insertCount; ++i) {
		int key = dis(gen);
		int value = dis(gen);
		lru.put(key, value);
	}
	auto endInsert = std::chrono::high_resolution_clock::now();
	auto insertDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endInsert - startInsert).count();

	// 查询操作性能测试
	auto startQuery = std::chrono::high_resolution_clock::now();
	for (int i = 0; i < queryCount; ++i) {
		int key = dis(gen);
		lru.get(key);
	}
	auto endQuery = std::chrono::high_resolution_clock::now();
	auto queryDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endQuery - startQuery).count();

	// 输出测试结果
	std::cout << "插入 " << insertCount << " 次操作耗时: " << insertDuration << " 毫秒" << std::endl;
	std::cout << "查询 " << queryCount << " 次操作耗时: " << queryDuration << " 毫秒" << std::endl;
}

using namespace Cache;

void testBasicOperations() {
	std::cout << "Running testBasicOperations...\n";

	LRU<int, std::string> cache(2);

	// 测试插入元素
	cache.put(1, "one");
	cache.put(2, "two");
	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";
	std::cout << "get(2): " << (cache.get(2).value_or("null")) << "\n";

	// 测试更新已有的元素
	cache.put(1, "ONE");
	std::cout << "get(1) after update: " << (cache.get(1).value_or("null")) << "\n";

	// 测试容量溢出时的淘汰机制
	cache.put(3, "three"); // 现在 key=2 应该被淘汰
	std::cout << "get(2) after eviction: " << (cache.get(2).value_or("null")) << "\n";
	std::cout << "get(3): " << (cache.get(3).value_or("null")) << "\n";
	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";
}

void testEvictionPolicy() {
	std::cout << "Running testEvictionPolicy...\n";
	LRU<int, std::string> cache(2);

	cache.put(1, "one");
	cache.put(2, "two");
	cache.get(1); // 访问 key=1，使 key=2 成为最久未使用
	cache.put(3, "three"); // 现在 key=2 应该被淘汰

	std::cout << "get(2) after eviction: " << (cache.get(2).value_or("null")) << "\n";
	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";
	std::cout << "get(3): " << (cache.get(3).value_or("null")) << "\n";
}

void testGetByReference() {
	std::cout << "Running testGetByReference...\n";
	LRU<int, std::string> cache(2);
	cache.put(1, "one");
	std::string value;

	if (cache.get(1, value)) {
		std::cout << "get(1) by reference: " << value << "\n";
	} else {
		std::cout << "get(1) failed\n";
	}

	if (cache.get(2, value)) {
		std::cout << "get(2) by reference: " << value << "\n";
	} else {
		std::cout << "get(2) failed (expected)\n";
	}
}

int main() {
	std::cout << "-------------------" << std::endl;
	testBasicOperations();
	std::cout << "-------------------" << std::endl;
	testEvictionPolicy();
	std::cout << "-------------------" << std::endl;
	testGetByReference();
	std::cout << "-------------------" << std::endl;
	performanceTest(10000,100000,50000);
	return 0;
}