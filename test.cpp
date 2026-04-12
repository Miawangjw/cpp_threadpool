#include <iostream>
#include "threadpool.h"


int sum(int a, int b, int c) {
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	return a + b + c;
}
int main(){
	ThreadPool pool;
	pool.setMode(ThreadPoolMode::MODE_CACHED);
	pool.start();
	std::future<int> res = pool.submitTask(sum, 1, 2, 3);

	auto res1 = pool.submitTask([](int b, int e) {
		long long sum = 0;
		for (int i = b; i <= e; i++) {
			sum += i;
		}
		return sum;
		}, 1, 1e8);

	auto res2 = pool.submitTask([](int b, int e) {
		long long sum = 0;
		for (int i = b; i <= e; i++) {
			sum += i;
		}
		return sum;
		}, 1, 1e8);

	auto res3 = pool.submitTask([](int b, int e) {
		long long sum = 0;
		for (int i = b; i <= e; i++) {
			sum += i;
		}
		return sum;
		}, 1, 1e8);

	auto res4 = pool.submitTask([](int b, int e) {
		long long sum = 0;
		for (int i = b; i <= e; i++) {
			sum += i;
		}
		return sum;
		}, 1, 1e8);
    std::cout << res.get() << std::endl;
	std::cout << res1.get() << std::endl;
	std::cout << res2.get() << std::endl;
	std::cout << res3.get() << std::endl;
    std::cout << res4.get() << std::endl;

}


