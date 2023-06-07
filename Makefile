LIB_SRC = Channel.cc EventLoop.cc Poller.cc Timer.cc TimerQueue.cc EventLoopThread.cc Acceptor.cc InetAddress.cc Socket.cc SocketsOps.cc TcpServer.cc TcpConnection.cc Buffer.cc EventLoopThreadPool.cc Connector.cc TcpClient.cc ./protobuf/codec.cc ./protobuf/query.pb.cc ./protobuf/rpc.pb.cc ./protobuf/rpcservice.pb.cc ./protobuf/RpcChannel.cc ./protobuf/RpcServer.cc ./protobuf/sudoku.pb.cc ./protobuf/median.pb.cc
# BINARIES = test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11 test12 test13
BINARIES = codectest dispatcher_lite_test dispatcher_test protobuf_server protobuf_client sudoku_server sudoku_client sorter collector
all: $(BINARIES)

include reactor.mk

# test1: test1.cc
# test2: test2.cc
# test3: test3.cc
# test4: test4.cc
# test5: test5.cc
# test6: test6.cc
# test7: test7.cc
# test8: test8.cc
# test9: test9.cc
# test10: test10.cc
# test11: test11.cc
# test12: test12.cc
# test13: test13.cc

codectest: ./protobuf/codec_test.cc
dispatcher_lite_test: ./protobuf/dispatcher_lite_test.cc
dispatcher_test: ./protobuf/dispatcher_test.cc
protobuf_server: ./protobuf/protobuf_server.cc
protobuf_client: ./protobuf/protobuf_client.cc
sudoku_server: ./protobuf/server.cc
sudoku_client: ./protobuf/client.cc
sorter: ./protobuf/sorter.cc
collector: ./protobuf/collector.cc
