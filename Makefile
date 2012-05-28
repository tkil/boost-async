all : async-signals2 asio-threads-1

% : %.cpp
	g++ -std=c++0x -O3 -Wall -pthread \
		$< -o $@ \
		-lboost_thread-mt -lboost_system-mt \
		-lpthread -lrt

async-signals2 asio-threads-1 : tlog.hpp
