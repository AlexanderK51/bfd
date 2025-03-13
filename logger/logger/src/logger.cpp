#include "logger.h"

#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace logger
{

static const std::unordered_map<Level, spdlog::level::level_enum> levels = {
    {Level::Error, spdlog::level::err},
    {Level::Warning, spdlog::level::warn},
    {Level::Info, spdlog::level::info},
    {Level::Debug, spdlog::level::debug},
};

Logger::Logger(const std::filesystem::path& directory,
               const std::string& fileName)
{
    if (!isExists(directory))
    {
        throw std::runtime_error("Found no logger directory: " +
                                 directory.string());
    }
    openFile(directory, fileName);
}

void Logger::openFile(const std::filesystem::path& directory,
                      const std::string& fileName)
{
    const std::filesystem::path file{fileName};
    const std::filesystem::path filePath{directory / file};
    // Внимание! Для приложения spdlog создает один объект для каждого имени
    // лога.
    //  Объект уничтожается при завершении работы приложения.
    const std::string loggerName = file.stem().string();
    logger_ = spdlog::get(loggerName);
    if (!logger_)
    {
        logger_ = spdlog::basic_logger_mt(loggerName, filePath.string());
    }
}

bool Logger::isExists(const std::filesystem::path& entry)
{
    return std::filesystem::exists(entry);
}

void Logger::setLevel(Level level) const
{
    if (levels.contains(level))
    {
        logger_->set_level(levels.at(level));
    }
}

void Logger::error(const std::string& message) const
{
    logger_->error(message);
}

void Logger::warning(const std::string& message) const
{
    logger_->warn(message);
}

void Logger::info(const std::string& message) const
{
    logger_->info(message);
}

void Logger::debug(const std::string& message) const
{
    logger_->debug(message);
    logger_->flush();
}

bool Logger::isDebugLevel() const
{
    return (logger_->level() == spdlog::level::level_enum::debug);
}

void Logger::flush()
{
    logger_->flush();
}

} // namespace logger
