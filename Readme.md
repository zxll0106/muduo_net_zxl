# 一个基于非阻塞 IO 和事件驱动的C++ 网络库
该项目是基于Reactor模式的网络库，采用one loop per thread+threadpool模型，每个线程只有一个事件循环EventLoop类，用于响应计时器和IO事件。每个Server类有自己的线程池，使用round-robin算法来选取池中的Eventloop.采用基于对象的设计风格，使用户在使用该网络库时不需要继承网络库中的类。为了使用将定时事件融入的IO复用框架中，使用timerfd避免处理信号。为了不用锁的情况下在线程间调配任务并保证线程安全，首先将该任务插入目标线程的任务队列，再向eventfd写入来唤醒线程。实现了Buffer来处理水平触发模式下的非阻塞I/O读写问题，避免忙等。
## 类图
![类图](https://github.com/zxll0106/muduo_net_zxl/blob/main/%E7%B1%BB%E5%9B%BE.png)
