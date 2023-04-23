cmake_minimum_required(VERSION 3.16)
project(tgbot)

include(CheckCXXSymbolExists)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DENABLE_LOGGER")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DENABLE_LOGGER -DDISABLE_LOGGER_TO_CONSOLE -DLOGGER_DBG_VIEW")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DENABLE_LOGGER -DDISABLE_LOGGER_TO_CONSOLE -DLOGGER_DBG_VIEW")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DENABLE_LOGGER -DDISABLE_LOGGER_TO_CONSOLE -DLOGGER_DBG_VIEW")

file(GLOB tgbot_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/logging.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/scoped_exit.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/use_awaitable.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/misc.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/strutil.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/default_cert.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/io_context_pool.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/async_connect.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/tgbot_services.hpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/animals.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/goat.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/http_last_modified.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/uncia.hpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/fileop.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/base_stream.hpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/http_proxy_client.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/socks_client.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/socks_enums.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/socks_error_code.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/tgbot/socks_io.hpp
)

file(GLOB tgbot_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/tgbot_services.cpp
)

add_executable(tgbot
	${tgbot_HEADERS}
	${tgbot_SRCS}
	)

target_link_libraries(tgbot
	Boost::asio
	Boost::beast
)

if (HAVE_IOCTL_SIOCGIFADDR)
	target_compile_definitions(tgbot PUBLIC HAVE_IOCTL_SIOCGIFADDR)
endif()
if (HAVE_GETIFADDRS)
	target_compile_definitions(tgbot PUBLIC HAVE_GETIFADDRS)
endif()

if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.29.30129 AND CMAKE_VERSION VERSION_GREATER 3.20.3)
	set_property(TARGET tgbot PROPERTY CXX_STANDARD 23)
	message(STATUS "tgbot set cxx standard: C++23")
else()
	set_property(TARGET tgbot PROPERTY CXX_STANDARD 20)
	message(STATUS "tgbot set cxx standard: C++20")
endif()

set(BASE_SOURCE_DIR
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(tgbot
	PUBLIC

	${BASE_SOURCE_DIR}
	${Boost_INCLUDE_DIRS}
)
