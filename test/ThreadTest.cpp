#include "Thread.h"
using namespace Cache;

void testThreadClass() {
	using Key = int;
	using Value = std::string;

	// 创建 Thread 对象
	Thread<Key, Value> thread(10, 1);

	// 测试 put 方法
	thread.put(1, "value1");
	thread.put(2, "value2");

	// 测试 get 方法
	std::optional<Value> result1 = thread.get(1);
	if (result1) {
		std::cout << "Get key 1: " << *result1 << std::endl;
	} else {
		std::cout << "Key 1 not found." << std::endl;
	}

	Value result2;
	if (thread.get(2, result2)) {
		std::cout << "Get key 2: " << result2 << std::endl;
	} else {
		std::cout << "Key 2 not found." << std::endl;
	}

	// 测试 commit 方法
	auto future = thread.commit([&thread]() {
	  thread.put(3, "value3");
	  std::optional<Value> result = thread.get(3);
	  if (result) {
		  std::cout << "Committed task: Get key 3: " << *result << std::endl;
	  } else {
		  std::cout << "Committed task: Key 3 not found." << std::endl;
	  }
	});

	// 等待任务完成
	future.wait();
}

int main() {
	testThreadClass();
	return 0;
}

