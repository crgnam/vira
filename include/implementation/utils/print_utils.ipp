#include <memory>
#include <string>
#include <ostream>
#include <csignal>

#include "indicators/progress_bar.hpp"
#include "indicators/cursor_control.hpp"
#include "termcolor/termcolor.hpp"


namespace vira::print {
    std::unique_ptr<indicators::ProgressBar> makeProgressBar(const std::string& title, indicators::Color color)
    {
        auto newBar = std::make_unique<indicators::ProgressBar>(
            indicators::option::BarWidth{ 50 },
            indicators::option::Start{ VIRA_INDENT + "[" },
            indicators::option::Fill{ "=" },
            indicators::option::Lead{ ">" },
            indicators::option::Remainder{ " " },
            indicators::option::End{ "]" },
            indicators::option::PostfixText{ title },
            indicators::option::ForegroundColor{ color },
            indicators::option::FontStyles{ std::vector<indicators::FontStyle>{indicators::FontStyle::bold} }
        );
        std::cout << std::flush;
        return newBar;
    };

    void updateProgressBar(std::unique_ptr<indicators::ProgressBar>& bar, const std::string& newMessage, float percentage)
    {
        bar->set_option(indicators::option::PostfixText{ newMessage });
        bar->set_progress(static_cast<size_t>(percentage));
        std::cout << std::flush;
    };

    void updateProgressBar(std::unique_ptr<indicators::ProgressBar>& bar, float percentage)
    {
        bar->set_progress(static_cast<size_t>(percentage));
        std::cout << std::flush;
    };


    // ================================== //
    // === Graceful Closing Functions === //
    // ================================== //
    void VIRA_RESET_CONSOLE() noexcept
    {
        // Operations to run at program exit:
        indicators::show_console_cursor(true);
        std::cout << termcolor::reset;
    };

    void signal_handler(int signal) noexcept
    {
        exit(signal);
    }

    void vira_terminate_handler() noexcept
    {
        VIRA_RESET_CONSOLE();
        std::cerr << "Unhandled exception. Terminating." << std::endl;
        std::abort();
    }

    void initializePrinting()
    {
        indicators::show_console_cursor(false);

        std::atexit(VIRA_RESET_CONSOLE);

        std::signal(SIGABRT, signal_handler);
        std::signal(SIGFPE, signal_handler);
        std::signal(SIGILL, signal_handler);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGSEGV, signal_handler);
        std::signal(SIGTERM, signal_handler);

        std::set_terminate(vira_terminate_handler);
    };
}