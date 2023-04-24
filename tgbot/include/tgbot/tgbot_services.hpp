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
#include "tgbot/goat.hpp"

#include <boost/assert.hpp>

#include <boost/json.hpp>
#include <boost/url.hpp>


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

		// 启动服务与停止服务.
		void start();
		void stop();

		// 接收消息处理 handler
		void message_handler(
			std::function<void(
				const std::string& text,
				int64_t chat_id,
				const boost::json::value& val)>
		);

		// 发送消息函数.
		net::awaitable<boost::json::value> send(
			const std::string& text,
			const int64_t& chat_id,
			int64_t reply_to_message_id = -1,
			std::string parse_mode = "",
			std::string message_thread_id = "",
			bool disable_web_page_preview = false,
			bool disable_notification = false,
			bool protect_content = false,
			bool allow_sending_without_reply = false
		);

		// 发送 action 消息函数.
		net::awaitable<void> send(
			const int64_t& chat_id,
			const std::string& action,
			std::string message_thread_id = ""
		);

	private:
		net::awaitable<void> scheduler();

		net::awaitable<boost::json::array>
		getphotos(const boost::json::array& arr);

	private:
		std::atomic_bool m_abort{ true };
		io_context_pool& m_ioc_pool;
		net::io_context& m_main_context;
		service_config m_service_config;
		std::function<void(
			const std::string&,
			int64_t,
			const boost::json::value&
			)> m_message_handler;
	};
}
