#include <stdexcept>
#include <memory>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <algorithm>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <experimental/filesystem>

#include <cstring> // strerror
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <strings.h>

#include <unistd.h> // ::close

#include <netdb.h>
#include <netinet/in.h>

using namespace std::literals;
namespace fs = std::experimental::filesystem;

/* Job object
 *  sockfd:     The stream file descritor to send the reply on.
 *  fileName:   The name of the file with data to be sent.
 */
struct Job
{
    int         sockfd;
    std::string fileName;

    Job(int sockfd, std::string const& fileName)
        : sockfd(sockfd)
        , fileName(fileName)
    {}
    ~Job()
    {
        if (sockfd)
        {
            ::close(sockfd);
        }
    }
};

class SimpleSocket
{
    int sockfd;
    public:
        SimpleSocket(int port)
        {
            sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0)
            {
                throw std::runtime_error("Failed to create socket");
            }

            struct sockaddr_in serv_addr;
            ::bzero((char *) &serv_addr, sizeof(serv_addr));

            serv_addr.sin_family        = AF_INET;
            serv_addr.sin_addr.s_addr   = INADDR_ANY;
            serv_addr.sin_port          = htons(port);

            if (::bind(sockfd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0)
            {
                throw std::runtime_error("Failed to bind to port: "s + std::strerror(errno));
            }
        }
        ~SimpleSocket()
        {
            ::close(sockfd);
        }
        int waitForConnection()
        {
            int result = ::listen(sockfd,5);
            if (result < 0)
            {
                throw std::runtime_error("Listen failed");
            }
            sockaddr_in     cli_addr;
            socklen_t       clilen      = sizeof(cli_addr);
            int             newsockfd   = ::accept(sockfd, reinterpret_cast<sockaddr*>(&cli_addr), &clilen);
            if (newsockfd < 0)
            {
                throw std::runtime_error("accept failed");
            }
            return newsockfd;
        }
};

/* The set of pages the can be returned */
class Pages
{
    public:
        // Note 1: page contains the server name and page.
        // See:    getPageFromRequest()
        // Note 2: A response is preset in the files <filaName>.head <filaName>.body
        // See:    workerThread()
        static std::unique_ptr<Job> getJob(int sockfd, std::string page)
        {
            std::string  fileName = std::string("./Pages/") + page;

            bool ok = fs::is_regular_file(fileName + ".head") && fs::is_regular_file(fileName + ".body");
            if (!ok)
            {
                // Note: Invalid will not clash with saved pages.
                //       because pages are in sub-directories based on
                //       the server name.
                return std::make_unique<Job>(sockfd, "./Pages/Invalid");
            }
            return std::make_unique<Job>(sockfd, fileName);
        }
};

/* Copy a buffer to stream */
void sendToClient(int sockfd, char* buffer, int size)
{
    std::size_t write = 0;
    while(write != size)
    {
        std::size_t out = ::write(sockfd, buffer + write, size - write);
        if (out == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
        {
            continue;
        }
        if (out == -1)
        {
            throw std::runtime_error("Failed to write to socket");
        }
        write += out;
    }
}

/* Copy a File to a stream */
void writeFile(int sockfd, std::string const& fileName)
{
    char                buffer[4096];
    std::ifstream       file(fileName.c_str());
    std::size_t         count;

    do
    {
        file.read(buffer, 4096);
        count = file.gcount();
        sendToClient(sockfd, buffer, count);
    }
    while(count > 0);
}
/* Read the request from the socket */
std::string getPageFromRequest(int connection)
{
    char buffer[4097];
    std::size_t read = 0;
    while(true)
    {
        std::size_t atual = ::read(connection, buffer + read, 4096 - read);
        if (atual == 0)
        {
            break;
        }
        if (atual == -1 && (errno == EAGAIN || errno == EINTR))
        {
            continue;
        }
        if (atual == -1)
        {
            throw std::runtime_error("Read Error");
        }
        read += atual;
        if (read == 4096)
        {
            break;
        }
    // hackery but for this simple job ¯\(0_0)/¯
        if ((read > 3) && buffer[read - 3] == '\n' && buffer[read - 2] == '\r' && buffer[read - 1] == '\n')
        {
            break;
        }
    }
    static std::string const marker = "Host: ";

    char* pageSt  = buffer + 4;
    char* pageEnd = std::find(pageSt, buffer + read, ' ');
    char* servSt  = std::search(buffer, buffer + read, marker.begin(), marker.end());
    char* servEnd = std::find(servSt + marker.size(), buffer + read, '\r');

    std::string req = std::string(servSt + marker.size(), servEnd) + std::string(pageSt, pageEnd);
    return std::string(pageSt, pageEnd);
}

void handleJob(std::unique_ptr<Job> job) {
    if (job->sockfd)
    {
        writeFile(job->sockfd, job->fileName + ".head");
        writeFile(job->sockfd, job->fileName + ".body");
    }
}

int main()
{
    std::vector<std::future<void>> pending_futures;
    SimpleSocket    socket(8100);

    int connection;
    while(connection = socket.waitForConnection())
    {
        std::string page = getPageFromRequest(connection);
        std::cout << "serving page " << page << std::endl;
        pending_futures.push_back(std::async(std::launch::async, handleJob, Pages::getJob(connection, page)));
    }

}