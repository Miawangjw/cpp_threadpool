#  C++ Thread Pool

一个基于 C++17 实现的高性能线程池，支持任务提交、返回值获取以及多种线程池运行模式，适用于高并发和多任务处理场景。

##  应用场景

- 高并发网络服务器
- 耗时任务异步处理
- 主从模式线程模型（Producer-Consumer）


## 核心特性

- 支持任意任务函数及参数提交
- 支持任务返回值（基于 `std::future`）
- 多线程安全任务调度
- 支持多种线程池模式（fixed / cached）

## 技术实现

- **可变参数模板 + 引用折叠**
  - 实现通用任务提交接口 `submitTask`
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

##  项目结构
cpp_threadpool/
├── include/ # 头文件
├── src/ # 源文件
├── test/ # 测试代码
├── README.md

##  使用示例

```cpp
ThreadPool pool;

// 提交任务
auto result = pool.submitTask([](int a, int b) {
    return a + b;
}, 10, 20);

// 获取结果
std::cout << result.get() << std::endl;
