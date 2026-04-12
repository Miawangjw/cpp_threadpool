#include <iostream>
#include "threadpool.h"

std::mutex mtx;
int sum(int a, int b, int c) {
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	return a + b + c;
}

class Logger : public TaskObserver {
public:
	void OnTaskFinished() override {
		std::lock_guard<std::mutex> lock(mtx);
		std::cout << "日志收到通知，任务完成" << std::endl;
	}
	void OnTaskFailed() override {
		std::lock_guard<std::mutex> lock(mtx);
		std::cout << "日志收到通知，任务失败" << std::endl;
	}
};

class Calculator : public TaskObserver {
public:
	void OnTaskFinished() override {
		std::lock_guard<std::mutex> lock(mtx);
		std::cout << "计算器收到通知，任务完成" << std::endl;
	}
	void OnTaskFailed() override {
		std::lock_guard<std::mutex> lock(mtx);
		std::cout << "计算器收到通知，任务失败" << std::endl;
	}
};

int main() {
	long long x, y;
	std::cin >> x >> y;
	ThreadPool pool;
	pool.setMode(ThreadPoolMode::MODE_CACHED);
	pool.start();
	auto logger_observer = std::make_shared<Logger>();
	auto calculator_observer1 = std::make_shared<Calculator>();
	auto calculator_observer2 = std::make_shared<Calculator>();

	auto res_logger = pool.submitTask([]() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			std::cout << "日志开始-----\n";
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		{
			std::lock_guard<std::mutex> lock(mtx);
			std::cout << "日志结束-----\n";
		}
		}, logger_observer);

	std::future<int> res_calc1 = pool.submitTask(sum, calculator_observer1, 1, 2, 3);

	auto res_calc2 = pool.submitTask([](int b, int e) {
		long long sum = 0;
		for (int i = b; i <= e; i++) {
			sum += i;
		}
		return sum;
		}, calculator_observer2, x, y);


	//std::cout << res_logger.get() << std::endl;
	{
		std::lock_guard<std::mutex> lock(mtx);
		std::cout << "1 + 2 + 3 = " << res_calc1.get() << std::endl;
		std::cout << "x到y的和: " << res_calc2.get() << std::endl;
	}

}


