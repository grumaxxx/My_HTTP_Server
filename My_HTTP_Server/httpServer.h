#pragma once

#define WIN32

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma warning(disable: 4996)
#else
typedef SOCKET ssize_t
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
#include <unordered_map>
#include "httpStruct.h"

#define MAX_SIZE 512

class httpServer 
{
public:

	explicit httpServer(uint16_t port);

	SOCKET acceptConnection() const;
	int mybind();
	int myrecv(std::array<char, MAX_SIZE>& buf) const;
	int mysend(std::array<char, MAX_SIZE>& buf, size_t len) const;
	void sendOK(int sock) const;

	httpData parseHttpReq(std::array<char, MAX_SIZE>& buf) const;

	std::pair<int, int> getHits(
		std::string user_hash,
		std::string addr_hash,
		std::string idstr);

	void process();

	void dispatch(const httpData& data);
	void dispatch(httpData&& data);
	void dispatch_thread_handler();

	~httpServer();
private:
	std::unordered_map <std::string, size_t> agentHit;
	std::unordered_map <std::string, size_t> addrHit;
	std::queue<httpData> _q;
	std::vector<std::thread> _threads;
	std::mutex _lock;
	std::condition_variable _cv;
	bool _quit = false;
	struct sockaddr_in _addr {};
	int _client{};
	int _socket{};

	fd_set master;
};

