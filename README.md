#  C++ Thread Pool

一个基于 C++17 实现的高性能线程池，支持任务提交、返回值获取，并引入观察者模式实现任务完成的异步通知机制，适用于高并发和多任务处理场景。

##  应用场景

- 高并发网络服务器
- 耗时任务异步处理
- 主从模式线程模型
- 异步事件通知系统

## 核心特性

- 支持任意任务函数及参数提交
- 支持任务返回值（基于 `std::future`）
- 多线程安全任务调度
- 支持多种线程池模式（fixed / cached）

## 技术实现


- **可变参数模板 + 引用折叠**
  - 实现通用任务提交接口 `submitTask`
  - ```cpp
    submitTask(func, observer, args...)
- **std::packaged_task + std::future**
  - 获取异步任务执行结果
- **任务队列（queue）+ 线程管理（unordered_map）**
  - 高效管理任务与线程生命周期
- **线程同步机制**
  - `std::mutex` + `std::condition_variable`
  - 实现任务提交与执行线程间通信
- **线程池模式**
  - `fixed`：固定线程数
  - `cached`：动态扩展线程
  -**支持任务失败通知**

##  使用示例

```cpp
class CalculatorObserver : public TaskObserver {
public:
	void OnTaskFinished() override {
		std::cout << "计算器收到通知，任务完成" << std::endl;
	}
	void OnTaskFailed() override {
		std::cout << "计算器收到通知，任务失败" << std::endl;
	}
};
ThreadPool pool;
pool.setMode(ThreadPoolMode::MODE_CACHED);
pool.start(4);
auto calculator_observer = std::make_shared<CalculatorObserver>();
// 提交任务
auto result = pool.submitTask([](int a, int b) {
    return a + b;
},calculator_observer, 10, 20);

// 获取结果
std::cout << result.get() << std::endl;
