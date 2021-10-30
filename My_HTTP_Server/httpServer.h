#pragma once

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning(disable: 4996)
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <thread>
#include <stdexcept>
#include <string>
#include <queue>
#include <array>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "httpStruct.h"

#define MAX_SIZE 512

class httpServer 
{
public:

	explicit httpServer(uint16_t port);
	size_t mybind();
	size_t myrecv(std::array<char, MAX_SIZE>& buf);
	size_t mysend(std::array<char, MAX_SIZE>& buf, size_t len);
	httpData parseHttpReq(std::array<char, MAX_SIZE>& buf);
	void process();

	void dispatch(const httpData& data);
	void dispatch(httpData&& data);
	void dispatch_thread_handler();

	~httpServer();
private:
	std::queue<httpData> _q;
	std::vector<std::thread> _threads;
	std::mutex _lock;
	std::condition_variable _cv;
	bool _quit = false;
	struct sockaddr_in _addr {};
	int _client{};
	int _socket{};

};

