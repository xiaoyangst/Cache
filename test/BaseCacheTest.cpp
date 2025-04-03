#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "LRU.h"
#include "LFU.h"
#include "ArcCache.h"

using namespace std;
using namespace Cache;

// 统一缓存初始化
std::vector<CachePolicy < int, std::string> *>

initializeCaches(int capacity) {
	return {new LRU<int, std::string>(capacity),
			new LFU<int, std::string>(capacity),
			new ArcCache<int, std::string>(capacity, 2)};
}

// 统一缓存测试逻辑
void performCacheOperations(CachePolicy<int, std::string> *cache, int operations, int hotDataNum, int coldDataNum) {
	std::random_device rd;
	std::mt19937 gen(rd());
	int hits = 0, get_ops = 0;

	for (int op = 0; op < operations; ++op) {
		int key = (op % 100 < 70) ? gen() % hotDataNum : hotDataNum + (gen() % coldDataNum);
		cache->put(key, "value" + std::to_string(key));
	}

	auto start_time = std::chrono::high_resolution_clock::now();
	for (int get_op = 0; get_op < operations; ++get_op) {
		int key = (get_op % 100 < 70) ? gen() % hotDataNum : hotDataNum + (gen() % coldDataNum);
		get_ops++;
		if (cache->get(key) != std::nullopt) {
			hits++;
		}
	}
	auto end_time = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end_time - start_time;

	std::cout << "命中率: " << (100.0 * hits / get_ops) << "%";
	std::cout << " | 访问耗时: " << elapsed.count() << " 秒" << std::endl;
}

// 热点数据测试
void testHotDataAccess(int capacity, int hotDataNum, int coldDataNum, int operations) {
	std::cout << "\n=== 测试场景1：热点数据访问测试 ===\n";
	auto caches = initializeCaches(capacity);
	for (auto &cache : caches) {
		performCacheOperations(cache, operations, hotDataNum, coldDataNum);
		delete cache;
	}
}

// 循环扫描测试
void testLoopPattern(int capacity, int loopSize, int operations) {
	std::cout << "\n=== 测试场景2：循环扫描测试 ===\n";
	auto caches = initializeCaches(capacity);
	std::random_device rd;
	std::mt19937 gen(rd());

	for (auto &cache : caches) {
		for (int key = 0; key < loopSize; ++key) {
			cache->put(key, "loop" + std::to_string(key));
		}
		int hits = 0, get_ops = 0, current_pos = 0;
		for (int op = 0; op < operations; ++op) {
			int key = (op % 100 < 60) ? current_pos : (gen() % loopSize);
			current_pos = (current_pos + 1) % loopSize;
			get_ops++;
			if (cache->get(key) != std::nullopt) {
				hits++;
			}
		}
		std::cout << "命中率: " << (100.0 * hits / get_ops) << "%" << std::endl;
		delete cache;
	}
}

// 工作负载剧烈变化测试
void testWorkloadShift(int capacity, int operations) {
	std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===\n";
	auto caches = initializeCaches(capacity);
	std::random_device rd;
	std::mt19937 gen(rd());
	int phase_length = operations / 5;

	for (auto &cache : caches) {
		for (int key = 0; key < 1000; ++key) {
			cache->put(key, "init" + std::to_string(key));
		}
		int hits = 0, get_ops = 0;
		for (int op = 0; op < operations; ++op) {
			int key;
			if (op < phase_length) key = gen() % 5;
			else if (op < 2 * phase_length) key = gen() % 1000;
			else if (op < 3 * phase_length) key = (op - 2 * phase_length) % 100;
			else if (op < 4 * phase_length) key = ((op / 1000) % 10) * 20 + (gen() % 20);
			else key = (gen() % 100 < 30) ? gen() % 5 : (gen() % 100 < 60) ? 5 + (gen() % 95) : 100 + (gen() % 900);
			get_ops++;
			if (cache->get(key) != std::nullopt) {
				hits++;
			}
			if (gen() % 100 < 30) {
				cache->put(key, "new" + std::to_string(key));
			}
		}
		std::cout << "命中率: " << (100.0 * hits / get_ops) << "%" << std::endl;
		delete cache;
	}
}

int main() {
	std::cout << "=== 缓存测试 1 ===" << std::endl;
	std::cout << "capacity " << 100 << " operations " << 10000 << std::endl;
	testHotDataAccess(100, 50, 500, 10000);
	testLoopPattern(100, 200, 10000);
	testWorkloadShift(100, 10000);

	std::cout << "\n=== 缓存测试 2 ===" << std::endl;
	std::cout << "capacity " << 200 << " operations " << 20000 << std::endl;
	testHotDataAccess(200, 100, 1000, 20000);
	testLoopPattern(300, 500, 20000);
	testWorkloadShift(300, 30000);

	std::cout << "\n=== 缓存测试 3 ===" << std::endl;
	std::cout << "capacity " << 500 << " operations " << 50000 << std::endl;
	testHotDataAccess(500, 200, 2000, 50000);
	testLoopPattern(500, 1000, 50000);
	testWorkloadShift(500, 50000);

	std::cout << "\n=== 缓存测试 4 ===" << std::endl;
	std::cout << "capacity " << 8000 << " operations " << 500000 << std::endl;
	testHotDataAccess(8000, 2000, 20000, 500000);
	testLoopPattern(8000, 1000, 500000);
	testWorkloadShift(8000, 500000);
	return 0;
}
