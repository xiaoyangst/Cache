#include <iostream>
#include <vector>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <unordered_set>
#include <stack>
#include <thread>
#include <atomic>
using namespace std;

struct Process {
  int pid_;
};

class processScheduling {
 public:
  processScheduling() : stop_(false) {

  }

  ~processScheduling() {
	  stop_.store(true);
	  cv_.notify_one();
	  if (worker_.joinable()) {
		  worker_.join();
	  }
  }

  void commit(Process p) {
	  lock_guard<mutex> lock(mtx_);
	  readyQueue_.push(p);
  }

  void run() {
	  worker_ = thread([this] {
		while (!stop_.load()) {
			unique_lock<mutex> lock(mtx_);
			cv_.wait(lock, [this] { return !readyQueue_.empty() || stop_; });
			if (stop_.load()) {
				break;
			}
			Process p = readyQueue_.front();
			readyQueue_.pop();
			lock.unlock();
			std::cout << "Process " << p.pid_ << " is running" << std::endl;
			
		}
	  });
  }

 private:
  atomic<bool> stop_;
  thread worker_;
  condition_variable cv_;
  mutex mtx_;
  queue<Process> readyQueue_;
};

int main() {

	return 0;
}

