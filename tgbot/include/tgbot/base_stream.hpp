//
// Copyright (C) 2022 Jack.
//
// Author: jack
// Email:  jack.wgm at gmail dot com
//

#pragma once

#include <boost/system/error_code.hpp>
#include <boost/variant2.hpp>

#include <boost/asio/socket_base.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace tgbot {
	namespace net = boost::asio;

	using tcp = net::ip::tcp;	// from <boost/asio/ip/tcp.hpp>

	template<typename... T>
	class base_stream : public boost::variant2::variant<T...>
	{
	public:
		template <typename S>
		explicit base_stream(S device)
			: boost::variant2::variant<T...>(std::move(device))
		{
			static_assert(std::is_move_constructible<S>::value
				, "must be move constructible");
		}
		~base_stream() = default;

		base_stream(const base_stream&) = delete;
		base_stream& operator=(base_stream const&) = delete;

		base_stream& operator=(base_stream&&) = default;
		base_stream(base_stream&&) = default;

		using executor_type = net::any_io_executor;

		executor_type get_executor()
		{
			return boost::variant2::visit([&](auto& t) mutable
				{ return t.get_executor(); }, *this);
		}

		template <typename MutableBufferSequence, typename ReadHandler>
		BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(ReadHandler,
			void(boost::system::error_code, std::size_t))
		async_read_some(const MutableBufferSequence& buffers,
			ReadHandler&& handler)
		{
			return boost::variant2::visit([&](auto& t) mutable
				{ return t.async_read_some(buffers,
					std::forward<ReadHandler>(handler)); }, *this);
		}

		template <typename ConstBufferSequence, typename WriteHandler>
		BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(WriteHandler,
			void(boost::system::error_code, std::size_t))
		async_write_some(const ConstBufferSequence& buffers,
			WriteHandler&& handler)
		{
			return boost::variant2::visit([&](auto& t) mutable
				{ return t.async_write_some(buffers,
					std::forward<WriteHandler>(handler)); }, *this);
		}

		tcp::endpoint remote_endpoint()
		{
			return boost::variant2::visit([&](auto& t) mutable
				{
					if constexpr (std::same_as<tcp::socket,
						std::decay_t<decltype(t)>>)
					{
						return t.remote_endpoint();
					}
					else
					{
						return t.lowest_layer().remote_endpoint();
					}
				}, *this);
		}

		void shutdown(net::socket_base::shutdown_type what,
			boost::system::error_code& ec)
		{
			boost::variant2::visit([&](auto& t) mutable
				{
					if constexpr (std::same_as<tcp::socket,
						std::decay_t<decltype(t)>>)
					{
						t.shutdown(what, ec);
					}
					else
					{
						t.lowest_layer().shutdown(what, ec);
					}
				}, *this);
		}

		void close(boost::system::error_code& ec)
		{
			boost::variant2::visit([&](auto& t) mutable
				{
					if constexpr (std::same_as<tcp::socket,
						std::decay_t<decltype(t)>>)
					{
						t.close(ec);
					}
					else
					{
						t.lowest_layer().close(ec);
					}
				}, *this);
		}
	};

	using ssl_stream = net::ssl::stream<tcp::socket>;
	using session_stream_type = base_stream<tcp::socket, ssl_stream>;


	inline session_stream_type
	instantiate_stream(session_stream_type& s)
	{
		return session_stream_type(tcp::socket(s.get_executor()));
	}

	inline session_stream_type
	instantiate_stream(net::any_io_executor executor)
	{
		return session_stream_type(tcp::socket(executor));
	}

	inline session_stream_type
	instantiate_stream(net::io_context& ioc)
	{
		return session_stream_type(tcp::socket(ioc));
	}

	inline session_stream_type
	instantiate_stream(tcp::socket&& s)
	{
		return session_stream_type(std::move(s));
	}

	inline session_stream_type
	instantiate_stream(tcp::socket&& s, net::ssl::context& sslctx)
	{
		return session_stream_type(ssl_stream(
			std::forward<tcp::socket>(s), sslctx));
	}

}
