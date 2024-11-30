#include "rvkpch.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "Framework/Log.h"

namespace RVK {
	std::shared_ptr<spdlog::logger> Log::m_coreLogger;
	std::shared_ptr<spdlog::logger> Log::m_appLogger;

	void Log::Init() {
		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("RVK.log", true));

		logSinks[0]->set_pattern("[%T] [%^%l%$] %n: %v");
		logSinks[1]->set_pattern("[%T] [%l] %n: %v");

		m_coreLogger = std::make_shared<spdlog::logger>("CORE", begin(logSinks), end(logSinks));
		spdlog::register_logger(m_coreLogger);
		m_coreLogger->set_level(spdlog::level::trace);
		m_coreLogger->flush_on(spdlog::level::trace);

		m_appLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(m_appLogger);
		m_appLogger->set_level(spdlog::level::trace);
		m_appLogger->flush_on(spdlog::level::trace);
	}
}