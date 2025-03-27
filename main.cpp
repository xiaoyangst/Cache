#include <iostream>
#include <memory>

class Test {
  int num_ = 0;
 public:
  void inc() { num_++; }

  int num() const { return num_; }
};

int main() {

	auto t1 = std::make_shared<Test>();
	auto t2 = t1;

	std::cout << "t1: " << t1.use_count() << std::endl;
	std::cout << "t2: " << t2.use_count() << std::endl;

	auto t3 = std::make_shared<Test>();
	t3->inc();

	std::swap(t1, t3);

	std::cout << "t1: " << t1.use_count() << " num: " << t1->num() << std::endl;
	std::cout << "t3: " << t3.use_count() << " num: " << t3->num() << std::endl;

	std::cout << "t2: " << t2.use_count() << " num: " << t2->num() << std::endl;

	return 0;
}
