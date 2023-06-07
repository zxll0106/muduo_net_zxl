#include "../EventLoop.h"
#include "../InetAddress.h"
#include "../datetime/Timestamp.h"
#include "../logging/Logging.h"
#include "RpcServer.h"
#include "median.pb.h"

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <numeric>
#include <unistd.h>
#include <vector>

using namespace muduo;

bool debug = false;

typedef ::boost::shared_ptr<median::Empty> EmptyPtr;
typedef ::std::function<void(const ::google::protobuf::Message *)>
    RpcDoneCallback;

class SorterImpl : public median::Sorter {
  public:
    SorterImpl() {
        xsubi_[0] = static_cast<uint16_t>(getpid());
        xsubi_[1] = static_cast<uint16_t>(gethostid());
        xsubi_[2] =
            static_cast<uint16_t>(Timestamp::now().microSecondsSinceEpoch());
        doGenerate(100, 0, 30);
    }
    virtual void Query(::google::protobuf::RpcController *controller,
                       const ::median::Empty *request,
                       ::median::QueryResponse *response,
                       ::google::protobuf::Closure *done) {
        LOG_INFO << "Query";
        response->set_count(data_.size());
        response->set_min(data_[0]);
        response->set_max(data_.back());
        response->set_sum(std::accumulate(data_.begin(), data_.end(), 0LL));
        done->Run();
    }

    virtual void Search(::google::protobuf::RpcController *controller,
                        const ::median::SearchRequest *request,
                        ::median::SearchResponse *response,
                        ::google::protobuf::Closure *done) {
        int64_t guess = request->guess();
        LOG_INFO << "Search " << guess;
        std::vector<int64_t>::iterator it =
            std::lower_bound(data_.begin(), data_.end(), guess);
        response->set_smaller(it - data_.begin());
        response->set_same(std::upper_bound(data_.begin(), data_.end(), guess) -
                           it);
        done->Run();
    }
    virtual void Generate(::google::protobuf::RpcController *controller,
                          const ::median::GenerateRequest *request,
                          ::median::Empty *response,
                          ::google::protobuf::Closure *done) {
        LOG_INFO << "Generate ";
        doGenerate(request->count(), request->min(), request->max());
        done->Run();
    }

    void doGenerate(int64_t count, int64_t min, int64_t max) {
        data_.clear();
        int64_t range = max - min;
        for (int64_t i = 0; i < count; i++) {
            int64_t value = min;
            if (range > 1) {
                value += nrand48(xsubi_) % range;
            }
            data_.push_back(value);
        }
        std::sort(data_.begin(), data_.end());
        if (debug) {
            std::copy(data_.begin(), data_.end(),
                      std::ostream_iterator<int64_t>(std::cout, " "));
            std::cout << std::endl;
        }
    }

  private:
    std::vector<int64_t> data_;
    unsigned short xsubi_[3];
};

int main(int argc, char *argv[]) {
    EventLoop loop;
    int port = argc > 1 ? atoi(argv[1]) : 5555;
    debug = argc > 2;
    InetAddress listenAddr(static_cast<uint16_t>(port));
    SorterImpl impl;
    RpcServer server(&loop, listenAddr);
    server.registerService(&impl);
    server.start();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
}