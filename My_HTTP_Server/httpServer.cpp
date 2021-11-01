#include "httpServer.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>
#include <functional>
#include "SHA1.h"

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
    // Create five threads and add them a dispatch handler
    for (size_t i = 0; i < _threads.size(); i++)
    {
        _threads[i] = std::thread(&httpServer::dispatch_thread_handler, this);
    }
    //Listen port
    int result = listen(_socket, SOMAXCONN);
    if (result < 0) {
        throw std::runtime_error("Failed to listen port");
    }

    master;
    FD_ZERO(&master);
    FD_SET(_socket, &master);
}

SOCKET httpServer::acceptConnection() const
{
    SOCKET newSock = accept(_socket, NULL, 0);
    if (newSock < 0) {
        throw std::runtime_error("Failed to accept port");
    }
    return newSock;
}

int httpServer::mybind() {
	return bind(
		_socket,
		reinterpret_cast<struct sockaddr*>(&_addr),
		sizeof(_addr)
	);
}
 
int httpServer::myrecv(std::array<char, MAX_SIZE> &buf) const
{
    return recv(
        _client,
        buf.data(),
        buf.size(),
        0
    );
}

int httpServer::mysend(std::array<char, MAX_SIZE>& buf, size_t len) const
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

void httpServer::sendOK(int sock) const
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    send(sock, response.c_str(), response.size(), NULL);
}

httpData httpServer::parseHttpReq(std::array<char, MAX_SIZE>& buf) const
{
    // Create string from array of char
    std::string str(buf.begin(), buf.end());
    // Check if it is GET request
    if (str.substr(0, 3) == "GET") {
        // Simple example, when we doesn't have anything after user-agent header
        std::string user_agent = str.substr(str.find("user-agent") + 12);
        // Request addr is betwen GET and HTTP/1.1
        auto httpVer = str.find("HTTP/1.1");
        std::string addr = str.substr(4, httpVer - 5);
        //Create pack of http data to check sum and cout it to console
        httpData pack{ _client, addr, user_agent };

        return pack;
    }
}

std::pair<int, int> httpServer::getHits(
    std::string user_hash,
    std::string addr_hash,
    std::string idstr)
{
    int agentHitCount{};
    int addrHitCount{};
    std::string agentMapKey = user_hash + idstr;
    if (agentHit.find(agentMapKey) != agentHit.end()) {
        agentHitCount = ++agentHit[agentMapKey];
    }
    else {
        agentHit[agentMapKey] = 1;
        agentHitCount = 1;
    }
    std::string addrMapKey = addr_hash + idstr;
    if (addrHit.find(addrMapKey) != addrHit.end()) {
        addrHitCount = ++addrHit[addrMapKey];
    }
    else {
        addrHit[addrMapKey] = 1;
        addrHitCount = 1;
    }
    return std::make_pair(agentHitCount, addrHitCount);
}

void httpServer::process()
{
    std::array<char, MAX_SIZE> buf{};

    bool running = true;
    while (running) {
        fd_set copy = master;

        // See who's talking to us
        int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

        // Loop through all the current connections / potential connect
        for (int i = 0; i < socketCount; i++)
        { 
            // Makes things easy for us doing this assignment
            SOCKET sock = copy.fd_array[i];

            _client = acceptConnection();
            myrecv(buf);
            dispatch(parseHttpReq(buf));

        }
    }
}

void httpServer::dispatch(const httpData& data)
{
    std::unique_lock<std::mutex> lock(_lock);
    // add httpData to queue
    _q.push(data);
    lock.unlock();
    _cv.notify_one();
}

void httpServer::dispatch(httpData&& data)
{
    std::unique_lock<std::mutex> lock(_lock);
    // add httpData to queue. if it's rvalue data, we can move it to queue
    _q.push(std::move(data));
    lock.unlock();
    _cv.notify_one();
}

void httpServer::dispatch_thread_handler()
{
    std::unique_lock<std::mutex> lock(_lock);
    do
    {
        // Wait elements in queue
         _cv.wait(lock, [this] {
            return (_q.size() || _quit);
            });

        if (!_quit && _q.size()) {

            auto data = _q.front();
            _q.pop();

            lock.unlock();
            
            std::ostringstream ss;

            ss << std::this_thread::get_id();

            std::string idstr = ss.str();
            //Ckeck sum of user-agent and address
            SHA1 checksum;
            //SHA1 request addr
            checksum.update(data._head);
            const std::string head_hash = checksum.final();
            //SHA1 user-agent information 
            checksum.update(data._userAgent);
            const std::string user_hash = checksum.final();

            int agentHitCount{};
            int addrHitCount{};
            // Ckeck count of addr and user-agent in this thread by adding a idstr to hash value
            std::tie(agentHitCount, addrHitCount) = getHits(user_hash, head_hash, idstr);

            std::cout
                << '<' << idstr << '>'
                << '<' << data._head << '-' << agentHitCount << '>'
                << '<' << head_hash << '>'
                << '<' << data._userAgent << '-' << addrHitCount << '>'
                << '<' << user_hash << '>'
                << std::endl;
            std::cout << idstr << std::endl;

            lock.lock();
        }
    } while (!_quit);
}

httpServer::~httpServer() {
    std::unique_lock<std::mutex> lock(_lock);
    _quit = true;
    lock.unlock();
    _cv.notify_all();

    // Join all active sockets
    for (size_t i = 0; i < _threads.size(); i++)
    {
        if (_threads[i].joinable())
        {
            _threads[i].join();
        }
    }

    closesocket(_socket);
}