#include <string>
#include <random>
#include <chrono>

#if 0

#include "LRUCache.h"

using namespace Cache;

void testBasicOperations() {
	LRUCache<std::string, int> cache(10, 2);
	cache.put("one", 1);
	cache.Print();

	std::cout << "*****************" << std::endl;

	cache.put("two", 2);
	cache.Print();

	std::cout << "get(one): " << cache.get("one").value_or(-1) << std::endl;
	std::cout << "get(two): " << cache.get("two").value_or(-1) << std::endl;

	cache.put("one", 11);
	std::cout << "get(one) after update: " << cache.get("one").value_or(-1) << std::endl;
	cache.Print();
}

void testSync() {
	LRUCache<std::string, int> cache(10, 2);
	cache.put("one", 1);
	cache.put("two", 1);
	cache.put("three", 1);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	cache.Print();

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	cache.Print();

	cache.put("four", 1);
	cache.put("five", 1);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	cache.Print();

	cache.put("six", 1);
	cache.put("seven", 1);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	cache.Print();
}

void benchmarkLRU(size_t numRequests, size_t threadNum, size_t cacheSize) {
	LRUCache<int, int> cache(cacheSize, threadNum);

	auto start = std::chrono::high_resolution_clock::now();

	std::vector<std::thread> workers;
	workers.reserve(threadNum);
	for (size_t t = 0; t < threadNum; ++t) {
		workers.emplace_back([&]() {
		  for (size_t i = 0; i < numRequests / threadNum; ++i) {
			  int key = i % cacheSize;
			  cache.put(key, key * 10);
			  cache.get(key);
		  }
		});
	}

	for (auto &worker : workers) {
		worker.join();
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;

	double qps = numRequests / duration.count();
	std::cout << "ThreadNum: " << threadNum << ", Total Requests: " << numRequests << ", Time: " << duration.count()
			  << "s, QPS: " << qps << std::endl;
}

void testQPS() {
	size_t numRequests = 500000;  // 总请求数
	size_t cacheSize = 100000;      // 缓存大小

	benchmarkLRU(numRequests, 1, cacheSize);

	benchmarkLRU(numRequests, 3, cacheSize);

	benchmarkLRU(numRequests, 5, cacheSize);

	benchmarkLRU(numRequests, 7, cacheSize);

	benchmarkLRU(numRequests, 8, cacheSize);

	benchmarkLRU(numRequests, 10, cacheSize);
}

void testCacheHitRateWithSync(size_t numRequests,
							  size_t threadNum,
							  size_t cacheSize,
							  size_t workingSetSize,
							  size_t syncInterval) {
	LRUCache<int, int> cache(cacheSize, threadNum);

	// 预热缓存
	for (size_t i = 0; i < cacheSize; ++i) {
		cache.put(i, i * 10);
	}

	std::atomic<size_t> hitCount{0};
	std::atomic<size_t> missCount{0};

	auto start = std::chrono::high_resolution_clock::now();

	std::vector<std::thread> workers;
	workers.reserve(threadNum);
	std::mutex syncMutex;  // 控制同步操作

	for (size_t t = 0; t < threadNum; ++t) {
		workers.emplace_back([&, t]() {
		  std::mt19937 rng(t); // 每个线程有独立随机数生成器
		  std::uniform_int_distribution<int> dist(0, workingSetSize - 1);

		  for (size_t i = 0; i < numRequests / threadNum; ++i) {
			  int key = dist(rng);  // 生成随机访问 key
			  if (cache.get(key).has_value()) {
				  hitCount++;
			  } else {
				  missCount++;
				  cache.put(key, key * 10);
			  }
		  }
		});
	}

	for (auto &worker : workers) {
		worker.join();
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;

	double hitRate = (double)hitCount / (hitCount + missCount) * 100.0;
	double missRate = 100.0 - hitRate;
	double requestsPerSecond = numRequests / duration.count();

	std::cout << "Total Requests: " << numRequests
			  << ", Cache Size: " << cacheSize
			  << ", Working Set Size: " << workingSetSize
			  << ", Sync Interval: " << syncInterval
			  << ", Time: " << duration.count() << "s"
			  << ", Hit Rate: " << hitRate << "%"
			  << ", Miss Rate: " << missRate << "%"
			  << ", Requests Per Second: " << requestsPerSecond << "\n";
}

void testHitRate() {
	size_t numRequests = 100000;  // 总请求数
	size_t threadNum = 4;         // 线程数
	size_t workingSetSize = 5000; // 工作集大小
	size_t syncInterval = 1000;   // 每多少次请求调用一次 syncCache()

	testCacheHitRateWithSync(numRequests, threadNum, 1000, workingSetSize, syncInterval);

	testCacheHitRateWithSync(numRequests, threadNum, 3000, workingSetSize, syncInterval);

	testCacheHitRateWithSync(numRequests, threadNum, 5000, workingSetSize, syncInterval);

	testCacheHitRateWithSync(numRequests, threadNum, 10000, workingSetSize, syncInterval);
}

#endif

int main() {

	return 0;
}