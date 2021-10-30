#ifndef DISPATCHER
#define DISPATCHER

#include <thread>
#include <functional>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <queue>
#include <mutex>
#include <iostream>
#include <string>
#include <array>
#include <condition_variable>
#include "httpServer.h"
#include "httpStruct.h"

#define MAX_SIZE 512

class dispatch_queue
{
public:
	dispatch_queue(std::string name, size_t thread_cnt = 1);
	~dispatch_queue();

	// dispatch and copy
	void dispatch(const httpData& pack);
	// dispatch and move
	void dispatch(httpData&& pack);

	//Parse HTTP
	void parseHTTP(httpData& pack);

	// Deleted operations
	dispatch_queue(const dispatch_queue& rhs) = delete;
	dispatch_queue& operator=(const dispatch_queue& rhs) = delete;
	dispatch_queue(dispatch_queue&& rhs) = delete;
	dispatch_queue& operator=(dispatch_queue&& rhs) = delete;

private:
	std::string _name;
	std::mutex _lock;
	std::vector<std::thread> _threads;
	std::queue<httpData> _q;
	std::condition_variable _cv;
	bool _quit = false;

	void dispatch_thread_handler(void);
};


#endif // !DISPATCHER