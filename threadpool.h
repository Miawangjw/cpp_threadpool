#ifndef THREADPOOL_H
#define THREADPOOL_H
/*
 *让线程池变得更方便
 * 1.使用可变参数
 * pool.submitTask(sum, 10, 20);
 * pool.submitTask(sum, 1, 2, 3, 4);
 * 2.使用c++11线程库，packaged_task async
 * 用future代替Result简化代码
 */
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>
#include <chrono>

const int TASK_MAX_THRESHOLD = INT16_MAX;
const int THREAD_MAX_THRESHOLD = 100;
const int THREAD_MAX_IDLE_TIME = 60;
//线程池支持模式

enum ThreadPoolMode {
	MODE_FIXED,	//固定数量的线程
	MODE_CACHED	//线程数量可动态增长
};

//线程类型
class Thread {
public:
	//线程启动函数
	using ThreadFunc = std::function<void(int)>;

	Thread(ThreadFunc func) : func_(func), threadId_(generateId_++){}

	~Thread() = default;

	//启动线程
	void start() {
		//创建一个线程并执行
		std::thread t(func_, threadId_);

		//分离线程 
		t.detach();
	}

	int getId() const {
		return threadId_;
	}

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_;	//保存线程id
};
int Thread::generateId_ = 0;

//线程池类型
class ThreadPool
{
public:
	ThreadPool()
		: initThreadSize_(4)
		, taskSize_(0)
		, taskQueueMaxThreshold_(TASK_MAX_THRESHOLD)
		, threadMaxThreshold_(THREAD_MAX_THRESHOLD)
		, poolMode_(ThreadPoolMode::MODE_FIXED)
		, curThreadSize_(0)
		, isRunning_(false)
		, idleThreadSize_(0)
	{}

	~ThreadPool() {
		isRunning_ = false;

		//等待所有现线程返回
		//两种状态：阻塞和正在执行中
		std::unique_lock<std::mutex> lock(taskQueueMtx_);
		//先上锁，在再唤醒所有线程，防止死锁
		notEmpty_.notify_all();
		exitCond_.wait(lock, [this]() {
			return curThreadSize_ == 0;
			});
	}

	//设置工作模式fixed或者cached
	void setMode(ThreadPoolMode mode) {
		//不允许中途更换状态
		if (checkRunningState()) {
			return;
		}
		this->poolMode_ = mode;
	}


	//设置task队列上限阈值
	void setTaskQueueMaxThreshHold(int threshold) {
		this->taskQueueMaxThreshold_ = threshold;
	}

	//设置cache模式下线程上限阈值
	void setThreadMaxSizeThreshold(int threadMaxSizeThreshold) {
		if (checkRunningState() || poolMode_ == ThreadPoolMode::MODE_FIXED) {
			return;
		}
		threadMaxThreshold_ = threadMaxSizeThreshold;
	}

	//给线程池提交任务
	//使用可变参数模板编程使得submitTask接收任意参数
	template<typename Func, typename... Args>
	auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
		//打包任务，放入任务队列
		using ReturnType = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
		);
		std::future<ReturnType> result = task->get_future();
		//获取锁
		std::unique_lock<std::mutex> lock(taskQueueMtx_);

		//线程通信 等待任务队列有空余
		//用户提交任务，最多等待1s，否则提交失败
		bool success = notFull_.wait_for(lock, std::chrono::seconds(1), [this]() {
			return taskQueue_.size() < (size_t)TASK_MAX_THRESHOLD;
			});

		if (!success) {
			// 构造一个“立即就绪”的 future
			std::packaged_task<ReturnType()> fallbackTask(
				[]() -> ReturnType {
					if constexpr (!std::is_void_v<ReturnType>) {
						return ReturnType();  // 非void返回默认值
					}
				}
			);

			auto fut = fallbackTask.get_future();
			fallbackTask();  // 立即执行，让 future ready
			return fut;
		}

		//如果有空余， 把任务放入任务队列中
		//taskQueue_.emplace(spTask);
		taskQueue_.emplace([task]() { (*task)(); });
		taskSize_.fetch_add(1);

		//因为放了新任务，队列不空，notify其他线程
		//lock.unlock();          // 提前释放锁优化
		notEmpty_.notify_one();

		//根据任务数量和空闲线程数量，动态增加或减少线程数量
		if (poolMode_ == ThreadPoolMode::MODE_CACHED
			&& taskSize_ > idleThreadSize_
			&& curThreadSize_ < threadMaxThreshold_) {
			//创建一个新线程
			std::unique_ptr<Thread>	uptrThread = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = uptrThread->getId();
			threads_.emplace(threadId, std::move(uptrThread));
			std::cout << "创建新线程:" << threadId << std::endl;
			threads_[threadId]->start(); //启动线程

			//更新线程数量
			curThreadSize_++;
			idleThreadSize_++;
		}
		return result;
	}

	//开启线程池
	void start(int initThreadSize = std::thread::hardware_concurrency()) {
		isRunning_ = true;

		this->initThreadSize_ = initThreadSize;
		this->curThreadSize_ = initThreadSize;
		//创建线程对象
		for (int i = 0; i < initThreadSize_; i++) {
			//把thread的this指针绑定到threadFunc,使用一个参数占位符
			std::unique_ptr<Thread>	uptrThread = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
			int threadId = uptrThread->getId();
			//这里必须要用std::move, 因为emplace_back需要拷贝，只能使用移动构造
			threads_.emplace(threadId, std::move(uptrThread));
		}
		//启动所有线程
		for (auto& [id, thread] : threads_) {
			thread->start();
			//每次启动一个空闲数量++;
			idleThreadSize_++;
		}
	}


	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	// 定义线程函数
	void threadFunc(int threadId) {
		auto lastTime = std::chrono::high_resolution_clock::now();

		while (true) {
			Task task;
			{
				//先获取锁
				std::unique_lock<std::mutex> lock(taskQueueMtx_);

				//cached模式下，超过60s线程空闲就应该回收
				//有任务必须先执行完所有任务在进入while回收
				while (taskQueue_.size() == 0) {
					if (!isRunning_) {
						threads_.erase(threadId);
						curThreadSize_--;
						idleThreadSize_--;
						exitCond_.notify_one();
						return;
					}
					if (poolMode_ == ThreadPoolMode::MODE_CACHED) {
						//每一秒返回一次
						//条件变量，超时返回获取当前精确时间
						if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
							auto now = std::chrono::high_resolution_clock::now();
							auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
							if (duration.count() >= THREAD_MAX_IDLE_TIME
								&& curThreadSize_ > initThreadSize_) {
								//回收线程
								//删除线程列表中的对应线程
								threads_.erase(threadId);
								curThreadSize_--;
								idleThreadSize_--;
								return;
							}
						}
					}
					else {
						//等待notEmpty
						//尝试获取任务	
						notEmpty_.wait(lock);
					}

				}
				//从任务队列中取一个任务
				task = taskQueue_.front();
				taskQueue_.pop();
				taskSize_.fetch_sub(1);
				idleThreadSize_--;

				//如果取完队列中仍然有剩余任务，通知所有线程
				if (taskQueue_.size() > 0) {
					notEmpty_.notify_one();
				}

				//通知可以提交任务
				notFull_.notify_one();
			}
			//作用域自动把锁释放,执行任务不需要上锁

			//当前线程负责执行这个任务
			if (task != nullptr) {
				task();
			}
			idleThreadSize_++;
			//更新线程执行完的时间
			lastTime = std::chrono::high_resolution_clock::now();
		}
	}

	//检查线程池运行状态
	bool checkRunningState() const {
		return isRunning_;
	}

private:
	//std::vector<std::unique_ptr<Thread>> threads_;	//线程列表
	std::unordered_map<int, std::unique_ptr<Thread>> threads_;	//线程列表
	std::atomic_int curThreadSize_;	//当前工作的线程数量
	std::atomic_int idleThreadSize_; //空闲线程数量
	int threadMaxThreshold_;	//线程的数量的上限阈值
	size_t initThreadSize_;	//初始线程数量

	using Task = std::function<void()>;
	std::queue<Task> taskQueue_;	//任务队列
	std::atomic_uint taskSize_;		//任务数量
	int taskQueueMaxThreshold_;	//任务队列的数量的上限阈值

	std::mutex taskQueueMtx_;	//保证任务队列的线程安全
	std::condition_variable notFull_; //表示队列不满
	std::condition_variable notEmpty_; //表示队列不空
	std::condition_variable exitCond_; //线程池退出条件变量,等待线程资源全部回收

	ThreadPoolMode poolMode_;
	std::atomic_bool isRunning_; //当前线程池工作状态
};






#endif