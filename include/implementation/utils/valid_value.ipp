#include <concepts>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::utils {
    template <typename T>
    T INVALID_VALUE()
    {
        if constexpr (IsFloatVec<T>) {
            return T{ std::numeric_limits<float>::infinity() };
        }
        else if constexpr (IsDoubleVec<T>) {
            return T{ std::numeric_limits<double>::infinity() };
        }
        else if constexpr (IsNumeric<T> && !IsFloat<T>) {
            return std::numeric_limits<T>::max();
        }
        if constexpr (IsFloat<T>) {
            return std::numeric_limits<T>::infinity();
        }
        else if constexpr (IsSpectral<T>) {
            return T{ std::numeric_limits<float>::infinity() };
        }
    };


    template <typename T>
    bool IS_VALID(T value)
    {
        // Check if invalid by equality:
        bool invalid = false;
        if constexpr (IsFloatingVec<T>) {
            invalid = !validVec<T>(value);
        }
        else if constexpr (IsNumeric<T> && !IsFloat<T>) {
            invalid = (value == std::numeric_limits<T>::max());
        }
        if constexpr (IsFloat<T>) {
            invalid = std::isinf(value) || std::isnan(value);
        }
        else if constexpr (IsSpectral<T>) {
            for (size_t i = 0; i < T::size(); ++i) {
                invalid = std::isinf(value[i]) || std::isnan(value[i]);
                if (invalid) {
                    break;
                }
            }
        }

        return !invalid;
    };

    template <typename T>
    bool IS_INVALID(T value)
    {
        return !IS_VALID<T>(value);
    };

    template <IsFloatingVec T>
    bool validVec(const T& vec)
    {
        bool invalid = false;
        if constexpr (T::length() == 1) {
            invalid = std::isinf(vec.x);
            invalid = invalid || std::isnan(vec.x);
        }
        else if constexpr (T::length() == 2) {
            invalid = std::isinf(vec.x) || std::isinf(vec.y);
            invalid = invalid || (std::isnan(vec.x) || std::isnan(vec.y));
        }
        else if constexpr (T::length() == 3) {
            invalid = std::isinf(vec.x) || std::isinf(vec.y) || std::isinf(vec.z);
            invalid = invalid || (std::isnan(vec.x) || std::isnan(vec.y) || std::isnan(vec.z));
        }
        else if constexpr (T::length() == 4) {
            invalid = std::isinf(vec.x) || std::isinf(vec.y) || std::isinf(vec.z) || std::isinf(vec.w);
            invalid = invalid || (std::isnan(vec.x) || std::isnan(vec.y) || std::isnan(vec.z) || std::isnan(vec.w));
        }

        return !invalid;
    };

    template<int C, int R, typename T, glm::qualifier Q>
    bool validMat(const glm::mat<C, R, T, Q>& matrix)
    {
        for (int row = 0; row < R; row++) {
            for (int col = 0; col < C; col++) {
                if (IS_INVALID<T>(matrix[col][row])) {
                    return false;
                }
            }
        }
        return true;
    };

    void validateNotNaN(double input, const std::string& parameterName)
    {
        if (std::isnan(input)) {
            throw std::invalid_argument(parameterName + " cannot be NaN");
        }
    };

    void validateFinite(double input, const std::string& parameterName)
    {
        if (std::isnan(input)) {
            throw std::invalid_argument(parameterName + " cannot be NaN");
        }

        if (std::isinf(input)) {
            throw std::invalid_argument(parameterName + " cannot be infinite");
        }
    }

    void validatePositive(double input, const std::string& parameterName)
    {
        validateFinite(input, parameterName);

        if (input < 0.0) {
            throw std::invalid_argument(parameterName + " must be positive (got " + std::to_string(input) + ")");
        }
    }

    void validatePositiveDefinite(double input, const std::string& parameterName)
    {
        validateFinite(input, parameterName);

        if (input <= 0.0) {
            throw std::invalid_argument(parameterName + " must be positive and non-zero (got " + std::to_string(input) + ")");
        }
    };

    void validateNormalized(double input, const std::string& parameterName)
    {
        validateFinite(input, parameterName);

        if (input < 0.0 || input > 1.0) {
            throw std::invalid_argument(parameterName + " must be between 0 and 1 (got " + std::to_string(input) + ")");
        }
    }

    template <IsSpectral TSpectral>
    void validatePositiveDefinite(TSpectral input, const std::string& parameterName)
    {
        for (auto& val : input) {
            vira::utils::validatePositiveDefinite(val, parameterName);
        }
    };
}