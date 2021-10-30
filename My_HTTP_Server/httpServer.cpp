#include "httpServer.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>


httpServer::httpServer(uint16_t port) :
    _threads(5)
{
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        std::cout << "Error" << std::endl;
        exit(1);
    }
	// Create socket
	if (_socket = socket(AF_INET, SOCK_STREAM, 0); _socket == -1) {
		throw std::runtime_error(strerror(errno));
	}
	// Set settings to bind port
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
	_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	// Bind socket
	if (this->mybind() == -1) {
		throw std::runtime_error(strerror(errno));
	}
    listen(_socket, SOMAXCONN);
    
    for (size_t i = 0; i < _threads.size(); i++)
    {
        _threads[i] = std::thread(&httpServer::dispatch_thread_handler, this);
    }
}

size_t httpServer::mybind()
{
	return bind(
		_socket,
		reinterpret_cast<struct sockaddr*>(&_addr),
		sizeof(_addr)
	);
}

// We need to know who was the sender to send info back
size_t httpServer::myrecv(std::array<char, MAX_SIZE> &buf)
{
    return recv(
        _client,
        buf.data(),
        buf.size(),
        0
    );
}

size_t httpServer::mysend(std::array<char, MAX_SIZE>& buf, size_t len)
{
    if (len >= buf.size()) {
        throw std::out_of_range("len >= buf.size()");
    }

    return send(
        _client,
        buf.data(),
        len,
        0
    );
}

httpData httpServer::parseHttpReq(std::array<char, MAX_SIZE>& buf)
{
    std::string str(buf.begin(), buf.end());
    if (str.substr(0, 3) == "GET") {
        std::string user_agent = str.substr(str.find("user-agent") + 12);
        auto httpVer = str.find("HTTP/1.1");
        std::string addr = str.substr(4, httpVer - 4);
        httpData pack{ _socket, addr, user_agent };

        return pack;
    }
}

void httpServer::process()
{

    while (_client = accept(_socket, NULL, 0)) {
        std::array<char, MAX_SIZE> buf{};

        if (auto len = myrecv(buf); len != 0) {
            //SEND 200 OK
            // 
            // 
            //Обработка запроса
            dispatch(parseHttpReq(buf));
        }
        else {
            break;
        }
    }
}

void httpServer::dispatch(const httpData& data)
{
    std::unique_lock<std::mutex> lock(_lock);
    _q.push(data);
    lock.unlock();
    _cv.notify_one();
}

void httpServer::dispatch(httpData&& data)
{
    std::unique_lock<std::mutex> lock(_lock);
    _q.push(std::move(data));
    lock.unlock();
    _cv.notify_one();
}

void httpServer::dispatch_thread_handler()
{
    std::unique_lock<std::mutex> lock(_lock);
    do
    {
         _cv.wait(lock, [this] {
            return (_q.size() || _quit);
            });

        if (!_quit && _q.size()) {

            auto data = _q.front();
            _q.pop();

            lock.unlock();

            //do something with data from queue
            // Parse and get SHA1

            lock.lock();
        }
    } while (!_quit);
}

httpServer::~httpServer() {
    std::unique_lock<std::mutex> lock(_lock);
    _quit = true;
    lock.unlock();
    _cv.notify_all();

    for (size_t i = 0; i < _threads.size(); i++)
    {
        if (_threads[i].joinable())
        {
            _threads[i].join();
        }
    }

    closesocket(_socket);
}