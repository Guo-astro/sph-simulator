#include <ctime>

#include <boost/format.hpp>

#include "logger.hpp"
#include "defines.hpp"
#include "exception.hpp"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace sph
{
std::string Logger::dir_name;
std::ofstream Logger::log_io;
bool Logger::open_flag = false;

void Logger::open(const std::string & output_dir)
{
    open(output_dir.c_str());
}

void Logger::open(const char * output_dir)
{
    struct stat st;
    bool is_mkdir = false;
    if(stat(output_dir, &st)) {
#ifdef _WIN32
        if(_mkdir(output_dir) == -1) {
#else
        if(mkdir(output_dir, 0775) == -1) {
#endif
            THROW_ERROR("cannot open directory");
        }
        is_mkdir = true;
    }

    // Log file creation disabled - console output only
    dir_name = output_dir;
    open_flag = true;
}

}