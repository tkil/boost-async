async-signals2 : async-signals2.cpp
	g++ -std=c++0x -O3 -Wall -pthread \
		async-signals2.cpp -o async-signals2 \
		-lboost_thread-mt -lboost_system-mt \
		-lpthread -lrt
