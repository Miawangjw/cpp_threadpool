# cpp_threadpool
项目适用于
1.高并发网络服务器
2.耗时任务处理
3.主从模式线程模型


线程池项目功能描述
1、基于可变参数模板编程和引用折叠原理，实现线程池 submitTask 接口，支持任意任务函数和任意参数的传递
2、使用 future 类型定制 submitTask 提交任务的返回值
3、使用 unordered_map 和 queue 容器管理线程对象和任务
4、基于条件变量 condition_variable 和互斥锁 mutex，实现任务提交线程与任务执行线程之间的通信机制
5、支持 fixed 和 cached 模式的线程池定制