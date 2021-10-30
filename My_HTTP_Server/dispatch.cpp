#include "dispatch.h"

dispatch_queue::dispatch_queue(std::string name, size_t thread_cnt) :
	_name{ std::move(name) }, _threads(thread_cnt)
{
	for (size_t i = 0; i < _threads.size(); i++)
	{
		_threads[i] = std::thread(&dispatch_queue::dispatch_thread_handler, this);
	}
}

dispatch_queue::~dispatch_queue()
{
	// Signal to dispatch threads that it's time to wrap up
	std::unique_lock<std::mutex> lock(_lock);
	_quit = true;
	lock.unlock();
	_cv.notify_all();

	// Wait for threads to finish before we exit
	for (size_t i = 0; i < _threads.size(); i++)
	{
		if (_threads[i].joinable())
		{
			_threads[i].join();
		}
	}
}

void dispatch_queue::dispatch(const httpData& pack)
{
	std::unique_lock<std::mutex> lock(_lock);
	_q.push(pack);

	// Manual unlocking is done before notifying, to avoid waking up
	// the waiting thread only to block again (see notify_one for details)
	lock.unlock();
	_cv.notify_one();
}

void dispatch_queue::dispatch(httpData&& pack)
{
	std::unique_lock<std::mutex> lock(_lock);
	_q.push(std::move(pack));

	// Manual unlocking is done before notifying, to avoid waking up
	// the waiting thread only to block again (see notify_one for details)
	lock.unlock();
	_cv.notify_one();
}

void dispatch_queue::dispatch_thread_handler(void)
{
	std::unique_lock<std::mutex> lock(_lock);

	do
	{
		// Wait until we have data or a quit signal
		_cv.wait(lock, [this] {
			return (_q.size() || _quit);
			});

		// after wait, we own the lock
		if (!_quit && _q.size())
		{
			auto data = _q.front();
			_q.pop();

			// unlock now that we're done messing with the queue
			lock.unlock();

			parseHTTP(data);

			lock.lock();
		}
	} while (!_quit);
}

void dispatch_queue::parseHTTP(httpData& pack)
{
		std::cout << std::this_thread::get_id() << std::endl;
		std::cout << pack._userAgent << std::endl;
		std::cout << pack._head << std::endl;
		std::cout << pack._sock_addr << std::endl;
}