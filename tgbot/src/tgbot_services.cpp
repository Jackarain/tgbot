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

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/name_generator_md5.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

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
	}

	void tgbot_services::stop()
	{
		BOOST_ASSERT(!m_abort && "service is stopped!");

		LOG_DBG << "service, stop";
		m_abort = true;
		m_ioc_pool.stop();
	}
}
