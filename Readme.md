# 一个基于非阻塞 IO 和事件驱动的C++ 网络库

## Reactor部分
![](https://github.com/zxll0106/muduo_net_zxl/blob/main/muduo.PNG)

### EventLoop类
one loop per thread每个线程只有一个EventLoop对象，EventLoop会记住所属线程的tid(`threadId_`).拥有EventLoop的线程是IO线程，主要功能是运行`loop()`，EventLoop对象的生存期和所属线程一样长。

`loop()`调用`Poller::poll()`获得当前活动事件的`Channel`列表，然后依次调用每个Channel的handleEvent()函数

`quit()`将`quit_`设为true，但是调用后不是立即起效，而是在`loop()`下次运行到`while(!quit)`

`runInLoop()`用户在其他线程调用`runInLoop()`，cb会被加入队列，IO线程会被唤醒来调用这个Functor。Io线程平时会阻塞在Poller::poll(),为了让IO线程能立刻执行用户回调，我们需要去唤醒它。传统方法是使用管道，其他线程往本线程管道里写入。
我们使用eventfd实现更高效地唤醒。
```
eventfd 是一个比 pipe 更高效的线程间事件通知
机制，一方面它比 pipe 少用一个 file descripor，
节省了资源；另一方面，eventfd 的缓冲区管理也
简单得多，全部“buffer” 只有定长8 bytes，不像
pipe 那样可能有不定长的真正 buffer。
```
```
void EventLoop::runInLoop(const Functor &cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor &cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    // if (callingPendingFunctors_) {
    //     printf("callingPendingFunctors!!\n");
    // }
    if (!isInLoopThread() || callingPendingFunctors_) {
        // printf("wakeup!!\n");
        wakeup();
    }
}
```
wakeupChannel_用于处理eventfd的可读事件，回调handleRead()函数，读走eventfd缓冲区的内容

`doPendingFunctors`不是对functor队列加锁后依次执行队列里的任务，而是通过swap()函数把队列的内容交换到另一个局部变量里，这样减少了临界区的长度，也避免死锁因为队列里的任务可能会再次调用到runInLoop()

### Channel类



每个`Channel`对象只属于一个EventLoop，因此每个Channel对象只属于某一个IO线程。`Channel`不拥有fd，不负责这个fd的生存期，只负责把不同的IO事件分发为不同的回调

`fd_`:负责的fd
`events_`：关心的IO事件
`revents_`：目前的活动事件，由Poller设置

`handleEvent()`当不同事件发生时，调用相应的回调

`enableReading()``enableWriting() disableWriting() disable()`修改`events`,调用`update()`

`set*Callback()`用户设置回调

### Poller类

IO复用poll的封装。每个EventLoop里有一个Poller

`poll()`调用::poll()系统调用获得当前活动的IO事件，

`fillActiveChannels()`遍历pollfds_数组，找到有活动事件的fd，把该fd对应的Channel队形填充到调用方传入的activeChannels，并设置channel对象的revents_

`updateChannel()`更新fd关注的事件，将Channel的events_的值赋给Poller的pollfds_里event。如果某个Channel不关心任何事件，就把pollfd.fd设为-1让::poll()忽略这个文件描述符

## Tcp网络库部分
### Acceptor类

用于`::accept()`新连接，并通过回调通知调用者。
每个TcpServer都有一个Acceptor。

在构造函数里完成创建socket，绑定Ip和端口的工作。
`listen()`：调用`::listen()`函数，成为用于监听的文件描述符。在调用enableReading()使channel对象所属EventLoop的poller对象监听该fd

`handleRead()`：当acceptSocket_可读时，回调这个函数，accept()新连接，再回调TcpServer::newConnection()函数

### TcpServer类
管理Acceptor类获得新连接TcpConnection

`start()`启动线程池，以及调用Acceptor::listen()

`newConnection()`

### TcpConnection类

### TimerQueue类

定时器队列，按到期时间排序。

使用std::set，key为pair<TimeStamp,Timer*>。
不使用map因为无法处理Timestamp相同的情况

当timerfd到期时，回调handleRead(),从set中取出到期定时器并删除，执行回调，






