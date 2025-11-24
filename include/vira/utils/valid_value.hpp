#ifndef VIRA_UTILS_VALID_VALUE_HPP
#define VIRA_UTILS_VALID_VALUE_HPP

#include "vira/constraints.hpp"

namespace vira::utils {
    template <typename T>
    T INVALID_VALUE();

    template <typename T>
    bool IS_VALID(T value);

    template <typename T>
    bool IS_INVALID(T value);

    template <IsFloatingVec T>
    bool validVec(const T& vec);

    template<int C, int R, typename T, glm::qualifier Q>
    bool validMat(const glm::mat<C, R, T, Q>& matrix);

    static inline void validateNotNaN(double input, const std::string& parameterName = "parameter");
    static inline void validateFinite(double input, const std::string& parameterName = "parameter");
    static inline void validatePositive(double input, const std::string& parameterName = "parameter");
    static inline void validatePositiveDefinite(double input, const std::string& parameterName = "parameter");
    static inline void validateNormalized(double input, const std::string& parameterName = "parameter");

    template <IsSpectral TSpectral>
    void validatePositiveDefinite(TSpectral input, const std::string& parameterName = "parameter");
};

#include "implementation/utils/valid_value.ipp"

#endif