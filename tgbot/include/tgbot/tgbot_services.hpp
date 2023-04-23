//
// stream_services.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2023 Jack (jack.arain at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include "tgbot/logging.hpp"
#include "tgbot/use_awaitable.hpp"
#include "tgbot/io_context_pool.hpp"
#include "tgbot/uncia.hpp"

#include <boost/assert.hpp>
#include <boost/json.hpp>


namespace tgbot {

	//////////////////////////////////////////////////////////////////////////

	struct service_config
	{
		std::string bot_token;
		std::string bot_server;
	};

	//////////////////////////////////////////////////////////////////////////

	namespace net = boost::asio;
	using asio_timer = net::steady_timer;
	using namespace asio_util;

	class tgbot_services
	{
	private:
		tgbot_services(const tgbot_services&) = delete;
		tgbot_services& operator=(const tgbot_services&) = delete;

		tgbot_services(tgbot_services&&) = delete;
		tgbot_services& operator=(tgbot_services&&) = delete;

	public:
		tgbot_services(io_context_pool& ioc_pool,
			const service_config& cfg);

		~tgbot_services() = default;

		void start();
		void stop();

	private:
		std::atomic_bool m_abort{ true };
		io_context_pool& m_ioc_pool;
		net::io_context& m_main_context;
		service_config m_service_config;
	};
}
