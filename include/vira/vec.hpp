#ifndef VIRA_VEC_HPP
#define VIRA_VEC_HPP

#include <ostream>
#include <iomanip>

// TODO REMOVE THIRD PARTY HEADERS:
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL

// Logic added because glm/gtx/hash.hpp detects incorrect _MSVC_LANG.  May be VS config problem?
#ifdef _WIN32
#   define message(ignore)
#   include "glm/gtx/hash.hpp"
#   undef message
#else
#   include "glm/gtx/hash.hpp"
#endif

#include "vira/constraints.hpp"

namespace vira {
    /**
     * @brief Alias for 4x4 glm::mat
     *
     * @tparam T The IsNumeric type to be stored in the mat
     */
    template <IsNumeric T>
    using mat4 = glm::mat<4, 4, T, glm::highp>;

    /**
     * @brief Alias for a 3x3 glm::mat
     *
     * @tparam T The IsNumeric type to be stored in the mat
     */
    template <IsNumeric T>
    using mat3 = glm::mat<3, 3, T, glm::highp>;

    /**
     * @brief Alias for a 2x2 glm::mat
     *
     * @tparam T The IsNumeric type to be stored in the mat
     */
    template <IsNumeric T>
    using mat2 = glm::mat<2, 2, T, glm::highp>;

    /**
     * @brief Alias for a 2x3 glm::mat
     *
     * @tparam T The IsNumeric type to be stored in the mat
     */
    template <IsNumeric T>
    using mat23 = glm::mat<3, 2, T, glm::highp>;

    template <int N, int M, IsNumeric T>
    using mat = glm::mat<N, M, T, glm::highp>;

    template <int N, IsNumeric T>
    using vec = glm::vec<N, T, glm::highp>;

    /**
     * @brief Alias for length 4 glm::vec
     *
     * @tparam T The IsNumeric type to be stored in the vec
     */
    template <IsNumeric T>
    using vec4 = glm::vec<4, T, glm::highp>;

    /**
     * @brief Alias for a length 3 glm::vec
     *
     * @tparam T The IsNumeric type to be stored in the vec
     */
    template <IsNumeric T>
    using vec3 = glm::vec<3, T, glm::highp>;

    /**
     * @brief Alias for a length 2 glm::vec
     *
     * @tparam T The IsNumeric type to be stored in the vec
     */
    template <IsNumeric T>
    using vec2 = glm::vec<2, T, glm::highp>;

    /**
     * @brief Alias for a lenght 1 glm::vec
     *
     * @tparam T The IsNumeric type to be stored in the vec
     */
    template <IsNumeric T>
    using vec1 = glm::vec<1, T, glm::highp>;

    typedef vec2<float> Pixel;
    typedef vec2<float> UV;
    typedef vec3<float> Normal;

    // Comparisons for sorting:
    template <typename T>
    struct vec2LessCompare
    {
        bool operator()(vec2<T> const& lhs, vec2<T> const& rhs) const {
            if (lhs.x < rhs.x) {
                return true;
            }
            else if (lhs.x == rhs.x) {
                return lhs.y < rhs.y;
            }
            else {
                return false;
            }
        }
    };


    // ======================= //
    // === Vector Concepts === //
    // ======================= //

    /**
     @brief Represents a glm::vec concept
    @details **Allowable Types:** Any type `T` that is a vira::vecN<float> for N = 2,3,4
    ```cpp
    template <typename T>
    concept IsVec = IsAnyOf<T, vec2<float>, vec3<float>, vec4<float>>;
    ```
    */
    template <typename T>
    concept IsFloatVec = IsAnyOf<T, vec2<float>, vec3<float>, vec4<float>>;

    /**
     @brief Represents a glm::vec concept
    @details **Allowable Types:** Any type `T` that is a vira::vecN<double> for N = 2,3,4
    ```cpp
    template <typename T>
    concept IsVec = IsAnyOf<T, vec2<double>, vec3<double>, vec4<double>>;
    ```
    */
    template <typename T>
    concept IsDoubleVec = IsAnyOf<T, vec2<double>, vec3<double>, vec4<double>>;


    template <typename T>
    concept IsFloatingVec = IsFloatVec<T> || IsDoubleVec<T>;

    /**
     @brief Represents a glm::vec concept
    @details **Allowable Types:** Any type `T` that satisfies either vira::IsFloatVec<T> or vira::IsDoubleVec<T>
    ```cpp
    template <typename T>
    concept IsVec = IsFloatVec<T> || IsDoubleVec<T>;
    ```
    */
    template <typename T>
    concept IsVec = requires(T x) {
        { glm::vec{ x } } -> std::same_as<T>;
    };

    template <typename T>
    concept IsUV = std::same_as<T, UV>;

    template <typename T>
    concept IsFloatVec3 = std::same_as<T, vec3<float>>;

    template <typename T>
    concept IsDoubleVec3 = std::same_as<T, vec3<double>>;

    template <typename T>
    concept IsMat = IsAnyOf<T, mat2<float>, mat2<double>, mat3<float>, mat3<double>, mat4<float>, mat4<double>>;


    template<typename T>
    concept IsVectorLike = requires
    {
        requires (IsFloatingVec<T> or std::same_as<T, std::vector<float>> or std::same_as<T, std::vector<double>>);
    };

    // ========================== //
    // === Operator Overloads === //
    // ========================== //
    template <IsVec T>
    bool operator< (const T& vec1, const T& vec2) {
        return glm::length(vec1) < glm::length(vec2);
    }

    template <IsVec T>
    bool operator> (const T& vec1, const T& vec2) {
        return glm::length(vec1) > glm::length(vec2);
    }

    template <IsVec T>
    bool operator <= (const T& vec1, const T& vec2) {
        return glm::length(vec1) <= glm::length(vec2);
    }

    template <IsVec T>
    bool operator >= (const T& vec1, const T& vec2) {
        return glm::length(vec1) >= glm::length(vec2);
    }

    template <IsVec T>
    bool operator == (const T& vec1, const T& vec2) {
        bool same = true;
        for (size_t i = 0; i < T::length(); ++i) {
            same = (same && (vec1[i] == vec2[i]));
        }
        return same;
    }


    template <IsFloatingVec T, IsNumeric N>
    T operator* (const T& vec, N numeric)
    {
        if constexpr (IsFloatVec<T>) {
            return vec * static_cast<float>(numeric);
        }
        else if constexpr (IsDoubleVec<T>) {
            return vec * static_cast<double>(numeric);
        }
    };

    template <IsFloatingVec T, IsNumeric N>
    T operator* (N numeric, const T& vec)
    {
        if constexpr (IsFloatVec<T>) {
            return static_cast<float>(numeric) * vec;
        }
        else if constexpr (IsDoubleVec<T>) {
            return static_cast<double>(numeric) * vec;
        }
    };

    template <IsFloatingVec T, IsNumeric N>
    T operator/ (const T& vec, N numeric)
    {
        if constexpr (IsFloatVec<T>) {
            return vec / static_cast<float>(numeric);
        }
        else if constexpr (IsDoubleVec<T>) {
            return vec / static_cast<double>(numeric);
        }
    };


    // ========================= //
    // === Vector operations === //
    // ========================= // 
    template <IsVec T>
    T normalize(const T& vec) {
        return glm::normalize(vec);
    }

    template <IsFloatVec T>
    float dot(const T& vec1, const T& vec2) {
        return glm::dot(vec1, vec2);
    }

    template <IsDoubleVec T>
    double dot(const T& vec1, const T& vec2) {
        return glm::dot(vec1, vec2);
    }

    template <IsFloatVec T>
    float length(const T& vec) {
        return glm::length(vec);
    }

    template <IsDoubleVec T>
    double length(const T& vec) {
        return glm::length(vec);
    }

    template <IsFloatVec3 T>
    T cross(const T& vec1, const T& vec2) {
        return glm::cross(vec1, vec2);
    }

    template <IsDoubleVec3 T>
    T cross(const T& vec1, const T& vec2) {
        return glm::cross(vec1, vec2);
    }

    template <IsMat T>
    T transpose(const T& mat) {
        return glm::transpose(mat);
    }

    template <IsVec T>
    T abs(const T& vec) {
        return glm::abs(vec);
    }



    // ========================= //
    // === Matrix Operations === //
    // ========================= //
    template <IsMat T>
    T inverse(const T& matrix) {
        return glm::inverse(matrix);
    }

    // TODO Implement glm::determinant wrapper

    template <IsFloat TFloat>
    vec3<TFloat> transformPoint(const mat4<TFloat>& transform, const vec3<TFloat>& point) {
        vec4<TFloat> homogeneous(point[0], point[1], point[2], TFloat(1));
        vec4<TFloat> result = transform * homogeneous;
        return vec3<TFloat>(result[0] / result[3], result[1] / result[3], result[2] / result[3]);
    }

    template <IsFloat TFloat>
    vec3<TFloat> transformDirection(const mat4<TFloat>& transform, const vec3<TFloat>& direction) {
        vec4<TFloat> homogeneous(direction[0], direction[1], direction[2], TFloat(0));
        vec4<TFloat> result = transform * homogeneous;
        return vec3<TFloat>(result[0], result[1], result[2]);
    }

    template <IsFloat TFloat>
    vec3<TFloat> transformNormal(const mat4<TFloat>& transform, const vec3<TFloat>& normal) {
        mat4<TFloat> inverse_transpose = transpose(inverse(transform));
        return normalize(transformDirection(inverse_transpose, normal));
    }



    // ========================== //
    // === Printing Overloads === //
    // ========================== //
    template<int C, int R, typename T>
    std::ostream& operator<<(std::ostream& os, const mat<C, R, T>& matrix) {
        // Set formatting
        std::ios oldState(nullptr);
        oldState.copyfmt(os);
        os << std::fixed << std::setprecision(6); // 6 decimal places
        os << std::setfill(' '); // Fill with spaces

        os << "[";
        for (int row = 0; row < R; row++) {
            if (row != 0) {
                os << " ";
            }
            os << "[ ";
            for (int col = 0; col < C; col++) {
                if (matrix[col][row] >= 0) {
                    os << " ";
                }
                os << std::setw(8) << matrix[col][row] << " ";
            }
            os << "]";
            if (row < (R - 1)) {
                os << "\n";
            }
        }
        os << "]\n";

        // Restore original stream state
        os.copyfmt(oldState);

        return os;
    }

    template<int N, typename T>
    std::ostream& operator<<(std::ostream& os, const vec<N, T>& vec) {
        // Set formatting
        std::ios oldState(nullptr);
        oldState.copyfmt(os);
        os << std::fixed << std::setprecision(6); // 6 decimal places
        os << std::setfill(' '); // Fill with spaces

        os << "[";
        for (int i = 0; i < N; ++i) {
            os << std::setw(8) << vec[i];
            if (i < (N - 1)) {
                os << " ";
            }
        }
        os << "]\n";

        // Restore original stream state
        os.copyfmt(oldState);

        return os;
    }
};


#endif