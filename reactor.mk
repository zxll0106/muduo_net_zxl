CXXFLAGS = -O0 -g  -Wall -I ../.. -pthread -ggdb3
LDFLAGS = -lpthread `pkg-config --cflags --libs protobuf` -lz
BASE_SRC = /home/zxl/Downloads/muduo_ZXL/datetime/Timestamp.cc /home/zxl/Downloads/muduo_ZXL/logging/Logging.cc /home/zxl/Downloads/muduo_ZXL/logging/LogStream.cc /home/zxl/Downloads/muduo_ZXL/thread/Thread.cc

$(BINARIES):
	g++ $(CXXFLAGS) -o $@ $(LIB_SRC) $(BASE_SRC) $(filter %.cc,$^) $(LDFLAGS)

clean:
	rm -f $(BINARIES) core

