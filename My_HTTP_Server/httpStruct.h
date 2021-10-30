#pragma once

#include <string>

struct httpData {
	httpData(int sock_addr, std::string head, std::string userAgent) :
		_sock_addr(sock_addr),
		_head(head),
		_userAgent(userAgent)
	{};
	int _sock_addr;
	std::string _head;
	std::string _userAgent;
};