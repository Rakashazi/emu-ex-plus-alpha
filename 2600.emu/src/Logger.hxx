#pragma once

// TODO: redirect into imagine logger system

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

    static void log(const string& message, Level level) {}

    static void error(const string& message) {}

    static void info(const string& message) {}

    static void debug(const string& message) {}

    void setLogParameters(int logLevel, bool logToConsole) {}
    void setLogParameters(Level logLevel, bool logToConsole) {}
};
