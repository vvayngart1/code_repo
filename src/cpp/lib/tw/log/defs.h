#pragma once

#include <iostream>

#include <tw/log/at.h>
#include <tw/log/function.h>
#include <tw/log/logger.h>

#define TW_LOG_AT               TW_AT "::" << TW_PRETTY_FUNCTION << ": "

#define LOGGER tw::log::Logger::instance().log()
#define LOGGER_INFO tw::log::Logger::instance().log(tw::log::eLogLevel::kInfo) << TW_LOG_AT
#define LOGGER_WARN tw::log::Logger::instance().log(tw::log::eLogLevel::kWarn) << TW_LOG_AT
#define LOGGER_ERRO tw::log::Logger::instance().log(tw::log::eLogLevel::kErro) << TW_LOG_AT

#define LOGGER_INFO_EMPHASIS LOGGER_INFO << "====================="
