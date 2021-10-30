#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>


#pragma warning(disable: 4996)



int main(int argc, char* argv[]) {

	char buff[512];

	const char* hello1 = "GET /AdGuard/test1.html HTTP/1.1"
						"user-agent: \"MAX\"";
	
	const char* hello2 = "GET /AdGuard/test2.html HTTP/1.1"
						"user-agent: \"OLEG\"";

	int UUID;
	std::string sUUID;

	//WSAStartup
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(54000);
	addr.sin_family = AF_INET;

	SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
	if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		std::cout << "Error: failed connect to server.\n";
		return 1;
	}
	std::cout << "Connected to HTTP Server\n";

	SOCKET Connection2 = socket(AF_INET, SOCK_STREAM, NULL);
	if (connect(Connection2, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		std::cout << "Error: failed connect to server.\n";
		return 1;
	}
	std::cout << "Connected to HTTP Server\n";
	
//	while(true) {
		send(Connection, hello1, strlen(hello1), NULL);
		//std::cout << "send hello1\n" << std::endl;
		send(Connection2, hello2, strlen(hello2), NULL);

		//std::cout << "send hello2\n" << std::endl;
//	}

	system("pause");
	return 0;
}