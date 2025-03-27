#include <string>
#include <chrono>

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

	cache.syncCache();

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	cache.Print();

	cache.put("four", 1);
	cache.put("five", 1);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	cache.syncCache();

	std::this_thread::sleep_for(std::chrono::seconds(3));

	std::cout << std::endl;
	std::cout << std::endl;
	std::cout << std::endl;

	cache.Print();

	cache.put("six", 1);
	cache.put("seven", 1);

	cache.syncCache();

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

	std::this_thread::sleep_for(std::chrono::seconds(3));

	benchmarkLRU(numRequests, 3, cacheSize);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	benchmarkLRU(numRequests, 5, cacheSize);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	benchmarkLRU(numRequests, 7, cacheSize);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	benchmarkLRU(numRequests, 8, cacheSize);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	benchmarkLRU(numRequests, 10, cacheSize);
}

void testCacheHitRateWithSync(size_t numRequests,
							  size_t threadNum,
							  size_t cacheSize,
							  size_t workingSetSize,
							  size_t syncInterval) {
	LRUCache<int, int> cache(cacheSize, threadNum);

	// 预填充缓存
	for (size_t i = 0; i < cacheSize; ++i) {
		cache.put(i, i * 10);
	}

	size_t hitCount = 0;
	size_t missCount = 0;

	auto start = std::chrono::high_resolution_clock::now();

	std::mutex mutex;

	std::vector<std::thread> workers;
	workers.reserve(threadNum);
	for (size_t t = 0; t < threadNum; ++t) {
		workers.emplace_back([&]() {
		  for (size_t i = 0; i < numRequests / threadNum; ++i) {
			  int key = i % workingSetSize; // 访问范围受 workingSetSize 限制
			  if (cache.get(key).has_value()) {
				  hitCount++;
			  } else {
				  missCount++;
				  cache.put(key, key * 10);
			  }

			  // 定期调用 syncCache()（比如每 syncInterval 次请求）
			  if (i % syncInterval == 0) {
				  std::lock_guard<std::mutex> lock_guard(mutex);
				  cache.syncCache();
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

	std::cout << "Total Requests: " << numRequests
			  << ", Cache Size: " << cacheSize
			  << ", Working Set Size: " << workingSetSize
			  << ", Sync Interval: " << syncInterval
			  << ", Time: " << duration.count() << "s"
			  << ", Hit Rate: " << hitRate << "%\n";
}

int main() {

	size_t numRequests = 100000;  // 总请求数
	size_t threadNum = 4;         // 线程数
	size_t cacheSize = 1000;      // 缓存大小
	size_t workingSetSize = 5000; // 工作集大小
	size_t syncInterval = 1000;   // 每多少次请求调用一次 syncCache()

	testCacheHitRateWithSync(numRequests, threadNum, cacheSize, workingSetSize, syncInterval);

	return 0;
}