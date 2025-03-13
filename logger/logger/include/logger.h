#pragma once

#include "spdlog/spdlog.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace logger
{

enum class Level : std::uint8_t  {
    Error,
    Warning,
    Info,
    Debug
};

class Logger
{
  public:
    Logger() = delete;
    explicit Logger(const std::filesystem::path& directory,
                    const std::string& file);
    Logger(const Logger&) = delete;
    Logger(Logger&&) noexcept = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) noexcept = delete;
    virtual ~Logger() = default;

    void setLevel(Level level) const;

    virtual void error(const std::string& message) const;
    virtual void warning(const std::string& message) const;
    virtual void info(const std::string& message) const;
    virtual void debug(const std::string& message) const;
    virtual bool isDebugLevel() const;

    void flush();

  private:
    static bool isExists(const std::filesystem::path& entry);
    void openFile(const std::filesystem::path& directory,
                  const std::string& fileName);

  private:
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace logger
