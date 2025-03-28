#include "LFU.h"
#include <iostream>
#include <cassert>

using namespace Cache;

void testBasicOperations() {
	LFU<std::string, int> cache(2);
	cache.put("one", 1);
	cache.put("two", 2);
	std::cout << "get(one): " << cache.get("one").value_or(-1) << std::endl;
	std::cout << "get(two): " << cache.get("two").value_or(-1) << std::endl;
	cache.put("one", 11);
	std::cout << "get(one) after update: " << cache.get("one").value_or(-1) << std::endl;
	cache.put("three", 3);
	cache.put("three",10);


	std::cout << "get(one): " << cache.get("one").value_or(-1) << std::endl;
	std::cout << "get(three): " << cache.get("three").value_or(-1) << std::endl;
	// two 应该 被淘汰
	assert(cache.get("two") == std::nullopt);
}

int main(){
	testBasicOperations();
	return 0;
}