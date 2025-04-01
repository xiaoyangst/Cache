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

void testKLRU(){
	KLru<int, std::string> cache(2, 3);

	cache.put(1, "one");
	cache.put(2, "two");

	// 查询 1，2 肯定不会有结果
	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";
	std::cout << "get(2): " << (cache.get(2).value_or("null")) << "\n";

	// 前面已经增加 1，2 的查询，应该就可以查到
	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";
	std::cout << "get(2): " << (cache.get(2).value_or("null")) << "\n";

	// 继续查询 1
	cache.put(1, "three");
	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";

	cache.put(4,"four");
	std::cout << "get(4): " << (cache.get(4).value_or("null")) << "\n";
	std::cout << "get(4): " << (cache.get(4).value_or("null")) << "\n";

	std::cout << "get(1): " << (cache.get(1).value_or("null")) << "\n";
	std::cout << "get(2): " << (cache.get(2).value_or("null")) << "\n";

}

int main() {
	testKLRU();
	return 0;
}