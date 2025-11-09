#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

namespace sph
{

class Logger {
    static std::string dir_name;
    static std::ofstream log_io;
    static bool open_flag;

    std::ostringstream m_msg;
    bool m_log_only;
public:
    Logger(bool log_only = false) : m_log_only(log_only) {}
    ~Logger() {
        // No log file output - console only
        if(!m_log_only) {
            std::cout << m_msg.str() << std::endl;
        }
    }

    static void open(const std::string & output_dir);
    static void open(const char * output_dir);
    static std::string get_dir_name() { return dir_name; }
    static bool is_open() { return open_flag; }

    template<typename T>
    Logger & operator<<(const T & msg) {
        m_msg << msg;
        return *this;
    }
};

// ✅ LOG LEVEL SYSTEM: Conditional logging based on build type
// Production build (NDEBUG defined): Only errors and critical messages
// Debug build (NDEBUG undefined): All debug messages enabled

// WRITE_LOG_ALWAYS: Always outputs to console and log (production + debug)
#define WRITE_LOG_ALWAYS sph::Logger()

#ifdef NDEBUG
    // Production: No debug logs, suppress output
    #define WRITE_LOG      if (false) sph::Logger()
    #define WRITE_LOG_ONLY if (false) sph::Logger(true)
#else
    // Debug: Enable all logging
    #define WRITE_LOG      sph::Logger()
    #define WRITE_LOG_ONLY sph::Logger(true)
#endif

}