#ifndef VIRA_UNITS_VECTOR_HPP
#define VIRA_UNITS_VECTOR_HPP

#include <array>
#include <cstddef>
#include <ostream>
#include <iomanip>
#include <cmath>
#include <utility> 

#include "vira/vec.hpp"
#include "vira/units/units.hpp"

namespace vira {
    template <typename T>
    concept IsVectorElem = units::IsUnit<T> || units::IsUnitResult<T>;

    template <IsVectorElem TUnit, size_t N>
    class Vector {
    public:
        //template <int T, int L, int M, int I, int O, int Num, int J, int A, int S>
        //operator Vector<
        template <units::IsUnitResult OtherUnit>
        Vector(const Vector<OtherUnit, N>& other) {
            static_assert(
                TUnit::dimension_T == OtherUnit::unit_base_type::dimension_T &&
                TUnit::dimension_L == OtherUnit::unit_base_type::dimension_L &&
                TUnit::dimension_M == OtherUnit::unit_base_type::dimension_M &&
                TUnit::dimension_I == OtherUnit::unit_base_type::dimension_I &&
                TUnit::dimension_O == OtherUnit::unit_base_type::dimension_O &&
                TUnit::dimension_N == OtherUnit::unit_base_type::dimension_N &&
                TUnit::dimension_J == OtherUnit::unit_base_type::dimension_J &&
                TUnit::dimension_A == OtherUnit::unit_base_type::dimension_A &&
                TUnit::dimension_S == OtherUnit::unit_base_type::dimension_S,
                "Cannot convert Vector<UnitResult> to Vector with incompatible unit dimensions"
                );

            // Convert each element using UnitResult's conversion operator
            for (size_t i = 0; i < N; ++i) {
                values[i] = other[i];
            }
        }

        template <IsNumeric T>
        Vector& operator=(const vira::vec<N, T>& vec) {
            static_assert(N < 4, "Can only convert vec1, vec2, vec3, or vec4");
            for (size_t i = 0; i < N; ++i) {
                values[i] = TUnit(static_cast<double>(vec[i]));
            }
            return *this;
        }

        // Convert TO glm::vec3
        template <IsFloat T>
        operator vira::vec<N, T>() const {
            static_assert(N < 4, "Can only convert vec1, vec2, vec3, or vec4");
            vira::vec<N, T> result;
            for (size_t i = 0; i < N; ++i) {
                result[i] = values[i].getValue();
            }
            return result;
        }

        // Convert FROM glm::vec3
        template <IsFloat T>
        explicit Vector(const vira::vec3<T>& vec) {
            static_assert(N < 4, "Can only convert vec1, vec2, vec3, or vec4");

            for (size_t i = 0; i < N; ++i) {
                values[i] = TUnit(static_cast<double>(vec[i]));
            }
        }

        // Constructors
        Vector() {
            // Initialize with default values
            for (size_t i = 0; i < N; ++i) {
                values[i] = TUnit{};
            }
        }

        // Initialize with same value
        explicit Vector(const TUnit& value) {
            for (size_t i = 0; i < N; ++i) {
                values[i] = value;
            }
        }

        // Initialize from values
        template<typename... Args,
            typename = std::enable_if_t<sizeof...(Args) == N &&
            (std::is_convertible_v<Args, TUnit> && ...)>>
            Vector(Args... args) : values{ static_cast<TUnit>(args)... } {}


        // Access operators
        TUnit& operator[](size_t index) { return values[index]; }
        const TUnit& operator[](size_t index) const { return values[index]; }

        // Basic vector operations
        TUnit dot(const Vector& other) const {
            TUnit result{};
            for (size_t i = 0; i < N; ++i) {
                result += values[i] * other.values[i];
            }
            return result;
        }

        // Vector magnitude
        auto magnitude() const {
            auto sum = (values[0] * values[0]);
            for (size_t i = 1; i < N; ++i) {
                sum += (values[i] * values[i]);
            }
            return std::sqrt(sum.getValue());
        }

        // Unary operations
        Vector operator-() const {
            Vector result;
            for (size_t i = 0; i < N; ++i) {
                result[i] = -values[i];
            }
            return result;
        }

        // Vector-Vector operations
        Vector& operator+=(const Vector& other) {
            for (size_t i = 0; i < N; ++i) {
                values[i] += other.values[i];
            }
            return *this;
        }

        Vector operator+(const Vector& other) const {
            Vector result = *this;
            result += other;
            return result;
        }

        Vector& operator-=(const Vector& other) {
            for (size_t i = 0; i < N; ++i) {
                values[i] -= other.values[i];
            }
            return *this;
        }

        Vector operator-(const Vector& other) const {
            Vector result = *this;
            result -= other;
            return result;
        }

        // Scalar multiplication
        template <IsNumeric T>
        Vector& operator*=(T scalar) {
            for (size_t i = 0; i < N; ++i) {
                values[i] *= scalar;
            }
            return *this;
        }

        template <IsNumeric T>
        Vector operator*(T scalar) const {
            Vector result = *this;
            result *= scalar;
            return result;
        }

        // Scalar division
        template <IsNumeric T>
        Vector& operator/=(T scalar) {
            for (size_t i = 0; i < N; ++i) {
                values[i] /= scalar;
            }
            return *this;
        }

        template <IsNumeric T>
        Vector operator/(T scalar) const {
            Vector result = *this;
            result /= scalar;
            return result;
        }

        // Division by unit (changes unit type)
        template <units::IsUnit OtherUnit>
        auto operator/(const OtherUnit& other) const {

            using ResultType = units::UnitResult<
                TUnit::dimension_T - OtherUnit::dimension_T,
                TUnit::dimension_L - OtherUnit::dimension_L,
                TUnit::dimension_M - OtherUnit::dimension_M,
                TUnit::dimension_I - OtherUnit::dimension_I,
                TUnit::dimension_O - OtherUnit::dimension_O,
                TUnit::dimension_N - OtherUnit::dimension_N,
                TUnit::dimension_J - OtherUnit::dimension_J,
                TUnit::dimension_A - OtherUnit::dimension_A,
                TUnit::dimension_S - OtherUnit::dimension_S
            >;

            Vector<ResultType, N> result{};
            for (size_t i = 0; i < N; ++i) {
                result[i] = ResultType{
                    values[i].getValue() / other.getValue(),
                    TUnit::scale_factor(), OtherUnit::scale_factor(),
                    TUnit::symbol(), OtherUnit::symbol(), ResultType::Operation::Divide
                };
            }

            return result;
        }

        // Multiplication by unit (changes unit type)
        template <units::IsUnit OtherUnit>
        auto operator*(const OtherUnit& other) const {

            using ResultType = units::UnitResult<
                TUnit::dimension_T + OtherUnit::dimension_T,
                TUnit::dimension_L + OtherUnit::dimension_L,
                TUnit::dimension_M + OtherUnit::dimension_M,
                TUnit::dimension_I + OtherUnit::dimension_I,
                TUnit::dimension_O + OtherUnit::dimension_O,
                TUnit::dimension_N + OtherUnit::dimension_N,
                TUnit::dimension_J + OtherUnit::dimension_J,
                TUnit::dimension_A + OtherUnit::dimension_A,
                TUnit::dimension_S + OtherUnit::dimension_S
            >;

            Vector<ResultType, N> result{};
            for (size_t i = 0; i < N; ++i) {
                result[i] = ResultType{
                    values[i].getValue() * other.getValue(),
                    TUnit::scale_factor(), OtherUnit::scale_factor(),
                    TUnit::symbol(), OtherUnit::symbol(), ResultType::Operation::Multiply
                };
            }

            return result;
        }

        // Size
        constexpr size_t size() const { return N; }

    protected:
        std::array<TUnit, N> values;
    };

    // Scalar * Vector operations
    template <IsNumeric T, units::IsUnit U, size_t N>
    Vector<U, N> operator*(T scalar, const Vector<U, N>& vec) {
        return vec * scalar;
    }

    // Cross product (3D vectors only)
    template <units::IsUnit U>
    Vector<U, 3> cross(const Vector<U, 3>& a, const Vector<U, 3>& b) {
        return Vector<U, 3>(
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]
        );
    }

    template <vira::units::IsLengthUnit T>
    using PositionVec = Vector<T, 3>;

    template <vira::units::IsVelocityUnit T>
    using VelocityVec = Vector<T, 3>;

    template <vira::units::IsAccelerationUnit T>
    using AccelerationVec = Vector<T, 3>;
};

// Stream operator for Vector types
template <vira::IsVectorElem T, size_t N>
std::ostream& operator<<(std::ostream& os, const vira::Vector<T, N>& input) {
    std::ios oldState(nullptr);
    oldState.copyfmt(os);

    os << std::fixed << std::setprecision(6);
    os << "[";
    for (size_t i = 0; i < N; ++i) {
        os << input[i].getValue();
        if (i < (N - 1)) {
            os << ", ";
        }
    }
    os << "] ";

    if constexpr (requires { input[0].getDerivedSymbol(); }) {
        os << input[0].getDerivedSymbol();
    }
    else {
        os << input[0].getSymbol();
    }

    os.copyfmt(oldState);
    return os;
}

#endif