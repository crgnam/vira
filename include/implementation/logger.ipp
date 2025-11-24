#include <sstream>
#include <fstream>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>

#include "plog/Log.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Appenders/RollingFileAppender.h"

namespace vira {

    // Function-local static accessors to avoid global constructor/destructor warnings
    inline bool& CrashLogger::get_initialized_flag() {
        static bool initialized = false;
        return initialized;
    }

    inline bool& CrashLogger::get_graceful_shutdown_flag() {
        static bool graceful_shutdown = false;
        return graceful_shutdown;
    }

    inline std::string& CrashLogger::get_log_file_path() {
        // Use a never-destructed string to avoid exit-time destructor warning
        static std::string* log_file_path = new std::string();
        return *log_file_path;
    }

    inline std::mutex& CrashLogger::get_log_mutex() {
        // Use a never-destructed mutex to avoid exit-time destructor warning
        static std::mutex* log_mutex = new std::mutex();
        return *log_mutex;
    }

    /**
     * Initialize the crash logger with a log file path
     * @param log_file_path Path to the log file (will be created if doesn't exist)
     * @param max_file_size_mb Maximum log file size in MB (default: 10MB)
     */
    inline void CrashLogger::initialize(const std::string& log_file_path, size_t max_file_size_mb) {
        if (get_initialized_flag()) {
            return; // Already initialized
        }

        initialize_internal(log_file_path, max_file_size_mb);
    }

    /**
     * Log an informational event
     */
    inline void CrashLogger::event(const std::string& message) {
        log_internal(Level::EVENT, message);
    }

    /**
     * Log a warning
     */
    inline void CrashLogger::warning(const std::string& message) {
        log_internal(Level::WARNING, message);
    }

    /**
     * Log an error
     */
    inline void CrashLogger::error(const std::string& message) {
        log_internal(Level::ERR, message);
    }

    /**
     * Log an event with formatting
     */
    template<typename... Args>
    void CrashLogger::event(const std::string& format, Args&&... args) {
        event(format_string(format, std::forward<Args>(args)...));
    }

    /**
     * Log a warning with formatting
     */
    template<typename... Args>
    void CrashLogger::warning(const std::string& format, Args&&... args) {
        warning(format_string(format, std::forward<Args>(args)...));
    }

    /**
     * Log an error with formatting
     */
    template<typename... Args>
    void CrashLogger::error(const std::string& format, Args&&... args) {
        error(format_string(format, std::forward<Args>(args)...));
    }

    /**
     * Call this on graceful program exit to delete the log file
     * This is automatically registered with atexit() but can be called manually
     */
    inline void CrashLogger::shutdown() {
        get_graceful_shutdown_flag() = true;

        if (get_initialized_flag()) {
            event("CrashLogger shutdown - graceful exit");

            // Give a moment for the final log to be written
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Delete the log file on graceful shutdown
            try {
                if (std::filesystem::exists(get_log_file_path())) {
                    std::filesystem::remove(get_log_file_path());
                }
            }
            catch (const std::exception& e) {
                // Can't really log this since we're shutting down
                std::cerr << "CrashLogger: Failed to delete log file: " << e.what() << std::endl;
            }

            get_initialized_flag() = false;
        }
    }

    /**
     * Check if the logger is initialized
     */
    inline bool CrashLogger::is_initialized() {
        return get_initialized_flag();
    }

    /**
     * Get default log filename based on executable name
     */
    inline std::string CrashLogger::get_default_log_filename() {
        try {
            auto exe_path = std::filesystem::current_path();
            return (exe_path / "crash.log").string();
        }
        catch (...) {
            return "crash.log";
        }
    }

    inline void CrashLogger::ensure_initialized() {
        if (!get_initialized_flag()) {
            initialize_internal(get_default_log_filename(), 10);
        }
    }

    inline void CrashLogger::initialize_internal(const std::string& log_file_path, size_t max_file_size_mb) {
        if (get_initialized_flag()) {
            return; // Already initialized
        }

        get_log_file_path() = log_file_path;

        try {
            // Initialize plog with immediate flush (no buffering)
            plog::init(plog::debug, get_log_file_path().c_str(), max_file_size_mb * 1024 * 1024, 1);
            get_initialized_flag() = true;

            // Register cleanup function to be called on normal exit
            // Suppress warning about potentially throwing function in extern "C" context
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 5039)
#endif
            std::atexit(cleanup_on_exit);
#ifdef _WIN32
#pragma warning(pop)
#endif

            // Log initialization
            event("CrashLogger auto-initialized - PID: {}", std::this_thread::get_id());

        }
        catch (const std::exception& e) {
            std::cerr << "CrashLogger: Failed to initialize: " << e.what() << std::endl;
        }
    }

    inline void CrashLogger::log_internal(Level level, const std::string& message) {
        ensure_initialized(); // Auto-initialize if needed

        if (!get_initialized_flag()) {
            return; // Failed to initialize
        }

        std::string formatted = format_message(level, message);
        write_log(formatted);
    }

    inline void CrashLogger::write_log(const std::string& message) {
        if (!get_initialized_flag()) return;

        std::lock_guard<std::mutex> lock(get_log_mutex());

        // Write through plog
        PLOG_INFO << message;

        // For immediate writes to survive crashes, we'll also write directly to file
        // This ensures the log persists even if plog's internal buffers aren't flushed
        try {
            std::ofstream file(get_log_file_path(), std::ios::app);
            if (file.is_open()) {
                file << message << std::endl;
                file.flush(); // Force immediate disk write
            }
        }
        catch (...) {
            // Ignore file write errors - plog will still have the log
        }
    }

    inline std::string CrashLogger::format_message(Level level, const std::string& message) {
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;

        // Use thread-safe localtime alternative on Windows
#ifdef _WIN32
        struct tm tm_buf;
        if (localtime_s(&tm_buf, &time_t) == 0) {
            oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        }
        else {
            oss << "UNKNOWN_TIME";
        }
#else
        struct tm* tm_ptr = std::localtime(&time_t);
        if (tm_ptr) {
            oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
        }
        else {
            oss << "UNKNOWN_TIME";
        }
#endif

        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        // Add level prefix
        const char* level_str;
        switch (level) {
        case Level::EVENT:   level_str = "EVENT"; break;
        case Level::WARNING: level_str = "WARN "; break;
        case Level::ERR:     level_str = "ERROR"; break;
        default:             level_str = "INFO "; break;
        }

        oss << " [" << level_str << "] " << message;
        return oss.str();
    }

    inline void CrashLogger::cleanup_on_exit() {
        if (!get_graceful_shutdown_flag()) {
            shutdown();
        }
    }

    template<typename... Args>
    std::string CrashLogger::format_string(const std::string& format, Args&&... args) {
        // Simple placeholder replacement - replace {} with arguments in order
        std::ostringstream oss;
        size_t pos = 0;
        std::string result = format;

        auto replace_next = [&](const auto& arg) {
            size_t placeholder_pos = result.find("{}", pos);
            if (placeholder_pos != std::string::npos) {
                std::ostringstream temp;
                temp << arg;
                result.replace(placeholder_pos, 2, temp.str());
                pos = placeholder_pos + temp.str().length();
            }
            };

        (replace_next(args), ...);
        return result;
    }
}