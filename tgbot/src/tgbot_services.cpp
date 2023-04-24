//
// tgbot_services.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2023 Jack (jack.arain at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include "tgbot/tgbot_services.hpp"
#include "tgbot/strutil.hpp"
#include "tgbot/scoped_exit.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexpansion-to-defined"
#endif

#include <fmt/ostream.h>
#include <fmt/printf.h>
#include <fmt/format.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <tuple>



namespace tgbot {

	namespace json = boost::json;

	namespace beast = boost::beast;	// from <boost/beast/http.hpp>
	namespace http = beast::http;

	//////////////////////////////////////////////////////////////////////////

	tgbot_services::tgbot_services(io_context_pool& ioc_pool,
		const service_config& cfg)
		: m_ioc_pool(ioc_pool)
		, m_main_context(ioc_pool.main_io_context())
		, m_service_config(cfg)
	{
	}

	void tgbot_services::start()
	{
		BOOST_ASSERT(m_abort && "service is running!");

		m_abort = false;

		net::co_spawn(m_main_context.get_executor(),
			[this]() mutable -> net::awaitable<void>
			{
				co_await scheduler();
				co_return;
			}, net::detached);
	}

	void tgbot_services::stop()
	{
		BOOST_ASSERT(!m_abort && "service is stopped!");

		LOG_DBG << "service, stop";
		m_abort = true;
		m_ioc_pool.stop();
	}

	void tgbot_services::message_handler(
		std::function<void(
			const std::string& text,
			int64_t chat_id,
			const boost::json::value& val)> handler)
	{
		m_message_handler = handler;
	}

	net::awaitable<boost::json::value>
	tgbot_services::send(
		const std::string& text,
		const int64_t& chat_id,
		int64_t reply_to_message_id /*= -1*/,
		std::string parse_mode /*= ""*/,
		std::string message_thread_id /*= ""*/,
		bool disable_web_page_preview /*= false*/,
		bool disable_notification /*= false*/,
		bool protect_content /*= false*/,
		bool allow_sending_without_reply /*= false */)
	{
		boost::json::object obj;

		obj["chat_id"] = chat_id;
		obj["text"] = text;
		if (reply_to_message_id != -1)
			obj["reply_to_message_id"] = reply_to_message_id;
		if (!parse_mode.empty())
			obj["parse_mode"] = parse_mode;
		if (!message_thread_id.empty())
			obj["message_thread_id"] = message_thread_id;
		if (disable_web_page_preview)
			obj["disable_web_page_preview"] = disable_web_page_preview;
		if (disable_notification)
			obj["disable_notification"] = disable_notification;
		if (protect_content)
			obj["protect_content"] = protect_content;
		if (allow_sending_without_reply)
			obj["allow_sending_without_reply"] = allow_sending_without_reply;

		auto content = boost::json::serialize(obj);
		std::string url;
		std::string host;

		animals::goat got(m_main_context.get_executor());

		{
			boost::urls::url u(m_service_config.bot_server);

			std::string token = "bot" +
				m_service_config.bot_token +
				"/sendMessage";

			u.set_path(token);
			url = std::string(u.buffer());
			host = u.host();
		}

		animals::http_request req{ http::verb::post, "", 11 };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, ANIMALS_VERSION_STRING);
		req.set(http::field::content_type, "application/json");
		req.body() = content;

		boost::system::error_code ec;
		boost::json::value result;
		auto resp = co_await got.async_perform(url, req, net_awaitable[ec]);
		if (ec)
		{
			LOG_ERR << "service, send message got: " << ec.message();
			co_return result;
		}

		if (resp.result() != animals::http::status::ok)
		{
			std::ostringstream oss;
			oss << resp;
			LOG_ERR << "service, send message bad: " << oss.str();
			co_return result;
		}

		auto response = animals::buffers_to_string(resp.body().data());
		result = boost::json::parse(response);

		co_return result;
	}

	net::awaitable<void>
	tgbot_services::send(
		const int64_t& chat_id,
		const std::string& action,
		std::string message_thread_id /*= "" */)
	{
		boost::json::object obj;

		obj["chat_id"] = chat_id;
		obj["action"] = action;

		if (!message_thread_id.empty())
			obj["message_thread_id"] = message_thread_id;

		auto content = boost::json::serialize(obj);
		std::string url;
		std::string host;

		animals::goat got(m_main_context.get_executor());

		{
			boost::urls::url u(m_service_config.bot_server);

			std::string token = "bot" +
				m_service_config.bot_token +
				"/sendChatAction";

			u.set_path(token);
			url = std::string(u.buffer());
			host = u.host();
		}

		animals::http_request req{ http::verb::post, "", 11 };
		req.set(http::field::host, host);
		req.set(http::field::user_agent, ANIMALS_VERSION_STRING);
		req.set(http::field::content_type, "application/json");
		req.body() = content;

		boost::system::error_code ec;
		boost::json::value result;
		auto resp = co_await got.async_perform(url, req, net_awaitable[ec]);
		if (ec)
		{
			LOG_ERR << "service, send action got: " << ec.message();
			co_return;
		}

		if (resp.result() != animals::http::status::ok)
		{
			std::ostringstream oss;
			oss << resp;
			LOG_ERR << "service, send action bad: " << oss.str();
			co_return;
		}

		auto response = animals::buffers_to_string(resp.body().data());
		LOG_DBG << response;

		co_return;
	}

	net::awaitable<void> tgbot_services::scheduler()
	{
		std::string url;
		std::string host;

		{
			boost::urls::url u(m_service_config.bot_server);

			std::string token = "bot" +
				m_service_config.bot_token +
				"/getUpdates";

			u.set_path(token);
			url = std::string(u.buffer());
			host = u.host();
		}

		std::string response;
		int64_t update_id = -1;

		animals::goat got(m_main_context.get_executor());
		asio_timer timer(m_main_context.get_executor());

		while (!m_abort)
		{
			boost::system::error_code ec;

			animals::http_request req{ http::verb::post, "", 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, ANIMALS_VERSION_STRING);
			req.set(http::field::content_type, "application/x-www-form-urlencoded");
			req.body() = "timeout=10&offset=" + std::to_string(update_id + 1);

			auto resp = co_await got.async_perform(url, req, net_awaitable[ec]);
			if (ec)
			{
				LOG_ERR << "service, http got: " << ec.message();
				got.reset();

				timer.expires_from_now(std::chrono::seconds(5));
				co_await timer.async_wait(net_awaitable[ec]);

				continue;
			}

			if (resp.result() != animals::http::status::ok)
			{
				std::ostringstream oss;
				oss << resp;
				LOG_ERR << "service, http bad: " << oss.str();
				got.reset();

				timer.expires_from_now(std::chrono::seconds(5));
				co_await timer.async_wait(net_awaitable[ec]);

				continue;
			}

			response = animals::buffers_to_string(resp.body().data());

			try {
				auto obj = boost::json::parse(response).as_object();
				if (!obj.contains("ok") || !obj.contains("result"))
					continue;

				auto results = obj["result"].as_array();
				for (auto& result_ref : results)
				{
					auto& result = result_ref.as_object();
					update_id = result["update_id"].as_int64();

					auto& message = result["message"].as_object();
					auto username = message["from"].as_object()["username"].as_string();
					auto chat_id = message["chat"].as_object()["id"].as_int64();

					std::string text;

					auto photos_ptr = message.if_contains("photo");
					if (photos_ptr)
					{
						auto photos = photos_ptr->if_array();
						if (!photos)
							continue;

						auto photo_paths = co_await getphotos(*photos);
						result["photos"] = photo_paths;

						LOG_DBG << username << ", id: " << update_id << ": " << boost::json::serialize(photo_paths);
					}
					else
					{
						text = message["text"].as_string();

						LOG_DBG << username << ", id: " << update_id << ": " << text;
					}

					if (m_message_handler)
						m_message_handler(text, chat_id, result);
				}
			}
			catch (const std::exception& e)
			{
				LOG_ERR << "service, exception: " << e.what();
			}
		}

		co_return;
	}

	net::awaitable<boost::json::array>
	tgbot_services::getphotos(const boost::json::array& arr)
	{
		boost::json::array ret;

		boost::urls::url u(m_service_config.bot_server);
		auto host = u.host();
		animals::goat get_photo(m_main_context.get_executor());

		for (auto& photo_ref : arr)
		{
			auto photo = photo_ref.if_object();
			if (!photo)
				continue;

			auto file_id_ptr = photo->if_contains("file_id");
			if (!file_id_ptr)
				continue;

			std::string token = "bot" +
				m_service_config.bot_token +
				"/getFile?file_id=" +
				std::string(file_id_ptr->as_string());

			u.set_path(token);
			auto photo_url = std::string(u.buffer());

			animals::http_request req{ http::verb::get, "", 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, ANIMALS_VERSION_STRING);

			boost::system::error_code ec;
			auto resp = co_await get_photo.async_perform(
				photo_url,
				req,
				net_awaitable[ec]);
			if (ec)
			{
				LOG_ERR << "service, get photo error: " << ec.message();
				break;
			}

			if (resp.result() != animals::http::status::ok)
			{
				std::ostringstream oss;
				oss << resp;
				LOG_ERR << "service, get photo bad: " << oss.str();
				continue;
			}

			auto response = animals::buffers_to_string(resp.body().data());
			try {
				auto obj = boost::json::parse(response).as_object();
				if (!obj.contains("ok") || !obj.contains("result"))
					continue;
				auto result_ptr = obj.if_contains("result");
				if (!result_ptr)
					continue;
				auto result_obj = result_ptr->as_object();
				auto file_path = result_obj.if_contains("file_path");
				if (!file_path)
					continue;

				std::string path = "file/bot" +
					m_service_config.bot_token +
					"/" +
					std::string(file_path->as_string());

				u.set_path(path);
				auto photo_url = std::string(u.buffer());
				ret.push_back(boost::json::value(photo_url));
			}
			catch (const std::exception& e)
			{
				LOG_ERR << "service, exception: " << e.what();
			}
		}

		co_return ret;
	}

}
