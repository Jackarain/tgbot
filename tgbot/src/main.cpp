//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2022 Jack (jack.arain at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#include "tgbot/logging.hpp"
#include "tgbot/scoped_exit.hpp"
#include "tgbot/use_awaitable.hpp"
#include "tgbot/misc.hpp"
#include "tgbot/strutil.hpp"

#include "tgbot/io_context_pool.hpp"
#include "tgbot/tgbot_services.hpp"

#include <boost/program_options.hpp>
namespace po = boost::program_options;


int main(int argc, char** argv)
{
	platform_init();

	std::string bot_token;
	std::string bot_server;
	std::string log_directory;
	bool disable_logs = false;

	// 解析命令行.
	po::options_description desc("Options");
	desc.add_options()
		("help,h", "Help message.")
		("bot-token", po::value<std::string>(&bot_token)->value_name("token"), "Telegram bot token id")
		("bot-server", po::value<std::string>(&bot_server)->default_value("https://api.telegram.org")->value_name("url"), "Telegram bot server url")
		("disable_logs", po::value<bool>(&disable_logs)->default_value(false), "Disable logs")
		("logs_path", po::value<std::string>(&log_directory)->value_name(""), "Logs dirctory.")
		;

	po::variables_map vm;
	po::store(
		po::command_line_parser(argc, argv)
		.options(desc)
		.style(po::command_line_style::unix_style
			| po::command_line_style::allow_long_disguise)
		.run()
		, vm);
	po::notify(vm);

	if (disable_logs)
	{
		util::toggle_logging();
		util::toggle_write_logging(true);
	}

	// 帮助输出.
	if (vm.count("help") || argc == 1)
	{
		std::cerr << desc;
		return EXIT_SUCCESS;
	}

	util::init_logging(log_directory);

	LOG_DBG << "Running...";

	tgbot::service_config cfg;
	cfg.bot_token = bot_token;
	cfg.bot_server = bot_server;

	using namespace asio_util;
	io_context_pool ioc_pool(std::thread::hardware_concurrency());

	tgbot::tgbot_services service(ioc_pool, cfg);
	service.message_handler([&service, &ioc_pool](
		const std::string& text,
		int64_t chat_id,
		const boost::json::value& val) {
		const auto& obj = val.as_object();
		const auto& message = obj.at("message").as_object();
		const auto message_id = message.at("message_id").as_int64();
		const auto& chat = message.at("chat").as_object();

		auto content = boost::json::serialize(val);
		LOG_DBG << content;

		net::co_spawn(ioc_pool.main_io_context(),
			[&service, chat_id, message_id]() -> net::awaitable<void> {
				co_await service.send(chat_id, "typing");
				co_await service.send("Hello", chat_id, message_id);
				co_return;
			}, net::detached);

		});
	service.start();

	ioc_pool.run();

	return EXIT_SUCCESS;
}
