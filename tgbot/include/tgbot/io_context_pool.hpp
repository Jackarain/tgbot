//
// io_context_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2019 Jack (jack dot wgm at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <vector>
#include <memory>
#include <thread>

#include <boost/asio/io_context.hpp>


namespace asio_util {

namespace net = boost::asio;

/// A pool of io_context objects.
class io_context_pool
{
	// c++11 noncopyable.
	io_context_pool(const io_context_pool&) = delete;
	io_context_pool& operator=(const io_context_pool&) = delete;

public:
	/// Construct the io_context pool.
	explicit io_context_pool(std::size_t pool_size)
		: next_io_context_(0)
	{
		if (pool_size == 0)
			throw std::runtime_error("io_context_pool size is 0");

		// Give all the io_contexts work to do so that their run() functions will
		// not exit until they are explicitly stopped.
		for (std::size_t i = 0; i < pool_size; ++i)
		{
			io_context_ptr ioc = std::make_shared<net::io_context>(
				BOOST_ASIO_CONCURRENCY_HINT_UNSAFE_IO);
			work_ptr work = std::make_shared<net::io_context::work>(*ioc);

			io_contexts_.emplace_back(std::move(ioc));
			work_.emplace_back(std::move(work));
		}

		// main_io_context_ do not have a worker on it, so main thread run() will
		// exit if no more work left
	}

	/// Run all io_context objects in the pool.
	inline void run()
	{
		// Create a pool of threads to run all of the io_contexts.
		std::vector<std::shared_ptr<std::thread>> threads;
		for (std::size_t i = 0; i < io_contexts_.size(); ++i)
		{
			std::shared_ptr<std::thread> thread(new std::thread(
				[this, i]() mutable {
					io_contexts_[i]->run();
				}));
			// set_thread_name(thread.get(), "round-robin io runner");
			threads.push_back(thread);
		}

		// main_io_context_ have no worker, so will exit if no more pending IO left.
		main_io_context_.run();

		// Wait for all threads in the pool to exit.
		for (auto& thread : threads)
			thread->join();
	}

	/// Stop all io_context objects in the pool.
	inline void stop()
	{
		// Explicitly stop all io_contexts.
		for (auto& io : work_)
			io.reset();
	}

	/// Get an io_context to use for a client.
	inline net::io_context& get_io_context()
	{
		// Use a round-robin scheme to choose the next io_context to use.
		net::io_context& io_context = *io_contexts_[next_io_context_];
		next_io_context_ = (next_io_context_ + 1) % io_contexts_.size();
		return io_context;
	}

	/// Get main_io_context_ to use.
	inline net::io_context& main_io_context()
	{
		return main_io_context_;
	}

	/// Get pool size.
	inline std::size_t pool_size() const
	{
		return io_contexts_.size();
	}

private:
	using io_context_ptr = std::shared_ptr<net::io_context>;
	using work_ptr = std::shared_ptr<net::io_context::work>;

	// main io_context that used to run the main logic
	net::io_context main_io_context_;

	/// The pool of io_contexts for client sockets
	std::vector<io_context_ptr> io_contexts_;

	/// The work that keeps the io_contexts running.
	std::vector<work_ptr> work_;

	std::size_t next_io_context_;
};


inline size_t default_max_transfer_size = 1024 * 1024;

class transfer_at_least_t
{
public:
	typedef std::size_t result_type;

	explicit transfer_at_least_t(std::size_t minimum)
		: minimum_(minimum)
	{
	}

	template <typename Error>
	std::size_t operator()(const Error& err, std::size_t bytes_transferred)
	{
		return (!!err || bytes_transferred >= minimum_)
			? 0 : default_max_transfer_size;
	}

private:
	std::size_t minimum_;
};

inline transfer_at_least_t transfer_at_least(std::size_t minimum)
{
	return transfer_at_least_t(minimum);
}

}
