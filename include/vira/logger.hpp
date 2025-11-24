#ifndef VIRA_LOGGER_HPP
#define VIRA_LOGGER_HPP

#include <string>
#include <mutex>

namespace vira {
    class CrashLogger {
    public:
        enum class Level {
            EVENT,
            WARNING,
            ERR
        };

        static void initialize(const std::string& log_file_path, size_t max_file_size_mb = 10);

        static void event(const std::string& message);
        static void warning(const std::string& message);
        static void error(const std::string& message);

        template<typename... Args>
        static void event(const std::string& format, Args&&... args);

        template<typename... Args>
        static void warning(const std::string& format, Args&&... args);

        template<typename... Args>
        static void error(const std::string& format, Args&&... args);

        static void shutdown();

        static bool is_initialized();

    private:
        static std::string get_default_log_filename();
        static void ensure_initialized();
        static void initialize_internal(const std::string& log_file_path, size_t max_file_size_mb);
        static void log_internal(Level level, const std::string& message);
        static void write_log(const std::string& message);
        static std::string format_message(Level level, const std::string& message);
        static void cleanup_on_exit();

        template<typename... Args>
        static std::string format_string(const std::string& format, Args&&... args);

        // Static member variables - using function-local statics to avoid exit-time destructors
        static bool& get_initialized_flag();
        static bool& get_graceful_shutdown_flag();
        static std::string& get_log_file_path();
        static std::mutex& get_log_mutex();
    };
}

#include "implementation/logger.ipp"

#endif