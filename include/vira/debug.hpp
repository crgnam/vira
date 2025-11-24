#ifndef VIRA_DEBUG_HPP
#define VIRA_DEBUG_HPP

#include <stdexcept>
#include <string>
#include <type_traits>
#include <concepts>

// DEBUGGING ONLY INCLUDES:
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#include "vira/utils/print_utils.hpp"
#endif

// DEBUGGING ONLY: Force single threaded execution
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
#include <tbb/global_control.h>

inline tbb::global_control& get_debug_control() {
    static tbb::global_control debug_single_thread_control(
        tbb::global_control::max_allowed_parallelism, 1);
    return debug_single_thread_control;
}

inline void init_debug_threading() {
    (void)get_debug_control(); // Force initialization
}
#endif


// ================== //
// === Debug Code === //
// ================== //
namespace vira::debug {

    constexpr bool enabled() {
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
        return true;
#else
        return false;
#endif
    }

    // Turn on debugging:
    inline void tbb_debug() {
#if defined(DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
        static std::once_flag flag;
        std::call_once(flag, []() {
            init_debug_threading();
            auto num_threads = tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism);
            std::cout << vira::print::printYellow("DEBUG Mode Detected. Running single threaded (TBB max parallelism: " + std::to_string(num_threads) + ")") << std::endl;
        });
#endif
    }


    inline void check_1d_bounds(size_t idx, size_t size) {
        if constexpr (enabled()) {
            if (idx >= size) {
                throw std::runtime_error("1D index " + std::to_string(idx) +
                    " out of bounds for size " + std::to_string(size));
            }
        }
    }

    inline void check_2d_bounds(int i, int j, int width, int height) {
        if constexpr (enabled()) {
            if (i < 0 || i >= width || j < 0 || j >= height) {
                throw std::runtime_error("2D index (" + std::to_string(i) + ", " +
                    std::to_string(j) + ") out of bounds for " +
                    std::to_string(width) + "x" + std::to_string(height) + " image");
            }
        }
    }

    template<typename T>
    inline void check_no_nan(const T& value, const char* message = "NaN detected") {
        if constexpr (enabled()) {
            if constexpr (IsSpectral<T> || IsVec<T>) {
                // Check all components
                for (size_t i = 0; i < value.size(); ++i) {
                    if (std::isnan(value[i])) {
                        throw std::runtime_error(message);
                    }
                }
            }
            else {
                // Single value
                if (std::isnan(value)) {
                    throw std::runtime_error(message);
                }
            }
        }
    }



    // Concept to identify pointer-like types
    template<typename T>
    concept PointerLike = requires(T t) {
        // Must be convertible to bool (for null checking)
        { static_cast<bool>(t) } -> std::convertible_to<bool>;
        // Must be comparable to nullptr
        { t == nullptr } -> std::convertible_to<bool>;
    } && (
        // Raw pointer
        std::is_pointer_v<T> ||
        // Smart pointers (have .get() method that returns a pointer)
        requires(T t) {
            { t.get() } -> std::convertible_to<typename T::pointer>;
    } ||
        // Other pointer-like types (have operator-> or operator*)
        requires(T t) {
            { t.operator->() };
    } ||
        requires(T t) {
            { *t };
    }
        );

    // Null pointer check - only accepts pointer-like types
    template<PointerLike T>
    inline void check_not_null(const T& ptr, const char* message = "Null pointer detected") {
        if constexpr (enabled()) {
            if (!ptr) {  // Uses operator bool() or comparison with nullptr
                throw std::runtime_error(message);
            }
        }
    }



    template<typename T>
    inline void check_range(T value, T min_val, T max_val, const char* message = "Value out of range") {
        if constexpr (enabled()) {
            if (value < min_val || value > max_val) {
                throw std::runtime_error(message);
            }
        }
    }
}

#endif