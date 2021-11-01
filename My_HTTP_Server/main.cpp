#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2tcpip.h>
#include "httpServer.h"

#ifdef WIN32
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 4996)
#endif // WIN32

#define PORT 54000

int main() {

	httpServer myServer{PORT};

	myServer.process();

	return EXIT_SUCCESS;
}