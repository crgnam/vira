#ifndef VIRA_UTILS_PRINT_UTILS_HPP
#define VIRA_UTILS_PRINT_UTILS_HPP

#include <memory>
#include <string>
#include <ostream>

// TODO REMOVE THIRD PARTY HEADERS:
#include "indicators/progress_bar.hpp"
#include "termcolor/termcolor.hpp"

namespace vira::print {

    const std::string VIRA_INDENT = "    ";
    const std::string VIRA_DOUBLE_INDENT = VIRA_INDENT + VIRA_INDENT;

    static inline std::unique_ptr<indicators::ProgressBar> makeProgressBar(const std::string& title, indicators::Color color = indicators::Color::green);

    static inline void updateProgressBar(std::unique_ptr<indicators::ProgressBar>& bar, const std::string& newMessage, float percentage);

    static inline void updateProgressBar(std::unique_ptr<indicators::ProgressBar>& bar, float percentage);


    class ColoredText {
    private:
        std::string message;
        std::ostream& (*color)(std::ostream&);

    public:
        ColoredText(const std::string& msg, std::ostream& (*clr)(std::ostream&))
            : message(msg), color(clr) {
        }

        friend std::ostream& operator<<(std::ostream& os, const ColoredText& ct) {
            return os << ct.color << ct.message << termcolor::reset;
        }
    };

    static inline ColoredText printGreen(const std::string& message) {
        return ColoredText(message, termcolor::green);
    }

    static inline ColoredText printRed(const std::string& message) {
        return ColoredText(message, termcolor::red);
    }

    static inline ColoredText printYellow(const std::string& message) {
        return ColoredText(message, termcolor::yellow);
    }

    static inline ColoredText printBlue(const std::string& message) {
        return ColoredText(message, termcolor::blue);
    }

    static inline ColoredText printOnBlue(const std::string& message) {
        return ColoredText(message, termcolor::on_blue);
    };

    static inline void printError(const std::string& errorMessage)
    {
        std::cerr << printRed("\nERROR: " + errorMessage) << std::endl << std::flush;
    }

    static inline void printWarning(const std::string& warningMessage)
    {
        std::cout << printYellow("\nWARNING: " + warningMessage) << std::endl << std::flush;
    }


    // ================================== //
    // === Graceful Closing Functions === //
    // ================================== //
    static inline void VIRA_RESET_CONSOLE() noexcept;

    [[noreturn]] static inline void signal_handler(int signal) noexcept;

    [[noreturn]] static inline void vira_terminate_handler() noexcept;

    static inline void initializePrinting();
}

namespace vira {
    namespace detail {
        // Check environment variable for default setting
        inline bool getDefaultPrintStatusSetting() {
#ifdef _WIN32
            char* env = nullptr;
            size_t len = 0;
            if (_dupenv_s(&env, &len, "VIRA_PRINT_STATUS") == 0 && env != nullptr) {
                std::string envStr(env);
                free(env);
                return envStr == "1" || envStr == "true" || envStr == "TRUE" || envStr == "on" || envStr == "ON";
            }
#else
            if (const char* env = std::getenv("VIRA_PRINT_STATUS")) {
                std::string envStr(env);
                return envStr == "1" || envStr == "true" || envStr == "TRUE" || envStr == "on" || envStr == "ON";
            }
#endif
            return false;
        }

        // Meyer's singleton pattern - thread-safe initialization in C++11+
        inline std::atomic<bool>& getPrintStatusFlag() {
            static std::atomic<bool> flag{ getDefaultPrintStatusSetting() };

            // Ensure initialization happens when flag is first accessed
            static std::once_flag init_flag;
            std::call_once(init_flag, []() {
                vira::print::initializePrinting();
                });

            return flag;
        }
    }

    inline void setPrintStatus(bool enabled) noexcept {
        detail::getPrintStatusFlag().store(enabled, std::memory_order_relaxed);
    }

    inline bool getPrintStatus() noexcept {
        return detail::getPrintStatusFlag().load(std::memory_order_relaxed);
    }

    inline void enablePrintStatus() noexcept {
        setPrintStatus(true);
    }

    inline void disablePrintStatus() noexcept {
        setPrintStatus(false);
    }
}

#include "implementation/utils/print_utils.ipp"

#endif