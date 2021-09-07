#pragma once

// TODO: redirect into imagine logger system

#include <string>

class Logger {

  public:

    enum class Level {
      ERR = 0, // cannot use ERROR???
      INFO = 1,
      DEBUG = 2,
      MIN = ERR,
      MAX = DEBUG
    };

  public:
    constexpr Logger() {}

    static Logger& instance()
    {
    	static Logger l;
    	return l;
    };

    static void log(const std::string& message, Level level) {}

    static void error(const std::string& message) {}

    static void info(const std::string& message) {}

    static void debug(const std::string& message) {}

    void setLogParameters(int logLevel, bool logToConsole) {}
    void setLogParameters(Level logLevel, bool logToConsole) {}
};
