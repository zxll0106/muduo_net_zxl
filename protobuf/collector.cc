#include "median.pb.h"

#include "../EventLoop.h"
#include "../EventLoopThread.h"
#include "../TcpClient.h"
#include "../thread/CountDownLatch.h"
#include "RpcChannel.h"
#include "kth.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <stdio.h>

using namespace muduo;

class RcpClient : boost::noncopyable {
  public:
    RcpClient(EventLoop *loop, const InetAddress &serverAddr)
        : loop_(loop), connectLatch_(NULL), client_(loop, serverAddr),
          channel_(new RpcChannel()), stub_(channel_.get()) {
        client_.setConnectionCallback(
            boost::bind(&RcpClient::onConnection, this, _1));
        client_.setMessageCallback(
            boost::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3));
    }

    void connect(CountDownLatch *connectLatch) {
        connectLatch_ = connectLatch;
        client_.connect();
    }

    median::Sorter::Stub *stub() { return &stub_; }

  private:
    void onConnection(const TcpConnectionPtr &conn) {
        if (conn->connected()) {
            channel_->setConnection(conn);
            if (connectLatch_) {
                connectLatch_->countDown();
                connectLatch_ = NULL;
            }
        }
    }

    EventLoop *loop_;
    CountDownLatch *connectLatch_;
    TcpClient client_;
    RpcChannelPtr channel_;
    median::Sorter::Stub stub_;
};

class Collector : boost::noncopyable {
  public:
    Collector(EventLoop *loop, const std::vector<InetAddress> &addresses)
        : loop_(loop), smaller_(0), same_(0), latch_(NULL) {
        for (const InetAddress &addr : addresses) {
            sorters_.push_back(new RcpClient(loop_, addr));
        }
    }
    void connect() {
        assert(!loop_->isInLoopThread());
        CountDownLatch latch(static_cast<int>(sorters_.size()));
        for (RcpClient &sorter : sorters_) {
            sorter.connect(&latch);
        }
        latch.wait();
    }

    void run() {
        result_.set_min(std::numeric_limits<int64_t>::max());
        result_.set_max(std::numeric_limits<int64_t>::min());
        getStats();
        LOG_INFO << "stats:\n" << result_.DebugString();
        const int64_t count = result_.count();
        if (count > 0) {
            LOG_INFO << "mean: "
                     << static_cast<double>(result_.sum()) /
                            static_cast<double>(count);
        }

        if (count <= 0) {
            LOG_INFO << "***** No median";
        } else {
            const int64_t k = (count + 1) / 2;
            std::pair<int64_t, bool> median =
                getKth(boost::bind(&Collector::search, this, _1, _2, _3), k,
                       count, result_.min(), result_.max());
            if (median.second) {
                LOG_INFO << "***** Median is " << median.first;
            } else {
                LOG_ERROR << "***** Median not found";
            }
        }
    }

  private:
    void getStats() {
        assert(!loop_->isInLoopThread());
        latch_ = new CountDownLatch(static_cast<int>(sorters_.size()));
        for (RcpClient &sorter : sorters_) {
            median::Empty req;
            median::QueryResponse *reps = new median::QueryResponse;
            sorter.stub()->Query(
                NULL, &req, reps,
                NewCallback(this, &Collector::QueryDone, reps));
        }
        latch_->wait();
    }

    void QueryDone(median::QueryResponse *reps) {
        assert(loop_->isInLoopThread());
        result_.set_count(reps->count() + result_.count());
        result_.set_sum(reps->sum() + result_.sum());
        if (reps->count() > 0) {
            if (reps->min() < result_.min()) {
                result_.set_min(reps->min());
            }
            if (reps->max() > result_.max()) {
                result_.set_max(reps->max());
            }
        }
        latch_->countDown();
    }

    void search(int64_t guess, int64_t *smaller, int64_t *same) {
        assert(!loop_->isInLoopThread());
        smaller_ = 0;
        same_ = 0;
        latch_ = new CountDownLatch(static_cast<int>(sorters_.size()));

        for (RcpClient &sorter : sorters_) {
            median::SearchResponse *reps = new median::SearchResponse;
            median::SearchRequest req;
            req.set_guess(guess);
            sorter.stub()->Search(NULL, &req, reps,
                                  google::protobuf::NewCallback(
                                      this, &Collector::SearchDone, reps));
        }
        latch_->wait();
        *smaller = smaller_;
        *same = same_;
    }

    void SearchDone(median::SearchResponse *reps) {
        assert(loop_->isInLoopThread());
        smaller_ += reps->smaller();
        same_ += reps->same();
        latch_->countDown();
    }

    EventLoop *loop_;
    boost::ptr_vector<RcpClient> sorters_;
    int64_t smaller_;
    int64_t same_;
    CountDownLatch *latch_;
    median::QueryResponse result_;
};

std::vector<InetAddress> getAddresses(int argc, char *argv[]) {
    std::vector<InetAddress> result;
    for (int i = 1; i < argc; ++i) {
        string addr = argv[i];
        size_t colon = addr.find(':');
        if (colon != string::npos) {
            string ip = addr.substr(0, colon);
            uint16_t port =
                static_cast<uint16_t>(atoi(addr.c_str() + colon + 1));
            result.push_back(InetAddress(ip, port));
        } else {
            LOG_ERROR << "Invalid address " << addr;
        }
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        LOG_INFO << "Starting";
        EventLoopThread loop;
        Collector collector(loop.startLoop(), getAddresses(argc, argv));
        collector.connect();
        LOG_INFO << "All connected";
        collector.run();
    } else {
        printf("Usage: %s sorter_addresses\n", argv[0]);
    }
}