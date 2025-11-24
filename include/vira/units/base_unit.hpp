#ifndef VIRA_UNITS_BASE_UNIT_HPP
#define VIRA_UNITS_BASE_UNIT_HPP

#include <ostream>
#include <string>
#include <type_traits>

#include "vira/constraints.hpp"

namespace vira::units {
    // ============================ //
    // === Forward declarations === //
    // ============================ //
    template <typename Derived, typename BaseUnit>
    class ConcreteUnit;

    template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
    class Unit;


    // =========================== //
    // === Unit Result Wrapper === //
    // =========================== //
    // Used to preserve source unit information
    template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
    class UnitResult {
    public:
        using unit_base_type = Unit<T, L, M, I, O, N, J, A, S>;
        using dimension_type = unit_base_type;

        enum class Operation { Multiply, Divide };

        UnitResult() = default;

        UnitResult(double value, double scale1, double scale2,
            const std::string& symbol1, const std::string& symbol2,
            Operation op = Operation::Multiply)
            : value_(value), scale1_(scale1), scale2_(scale2),
            symbol1_(symbol1), symbol2_(symbol2), operation_(op) {
        }

        // Convert to any compatible unit type
        template <typename TargetUnit>
        operator TargetUnit() const {
            // Check if dimensions match
            static_assert(
                TargetUnit::dimension_T == T &&
                TargetUnit::dimension_L == L &&
                TargetUnit::dimension_M == M &&
                TargetUnit::dimension_I == I &&
                TargetUnit::dimension_O == O &&
                TargetUnit::dimension_N == N &&
                TargetUnit::dimension_J == J &&
                TargetUnit::dimension_A == A &&
                TargetUnit::dimension_S == S,
                "Cannot convert UnitResult to incompatible unit type"
                );

            // Calculate the conversion factor
            double result_value = value_;

            // Apply the appropriate conversion based on the target unit's scale factor
            return TargetUnit(result_value * compute_scale() / TargetUnit::scale_factor());
        }

        double getValue() const { return value_; }
        std::string getSymbol() const {
            if (operation_ == Operation::Multiply)
                return symbol1_ + "*" + symbol2_;
            else
                return symbol1_ + "/" + symbol2_;
        }

        // This exposes the scale computation to be used by other conversions
        double compute_scale() const {
            if (operation_ == Operation::Multiply)
                return scale1_ * scale2_;
            else
                return scale1_ / scale2_;
        }

    private:
        double value_ = 0;
        double scale1_ = 1;
        double scale2_ = 1;
        std::string symbol1_ = "";
        std::string symbol2_ = "";
        Operation operation_;
    };




    // ============================= //
    // === Primary Unit Template === //
    // ============================= //
    template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
    class Unit {
    public:
        using unit_base_type = Unit<T, L, M, I, O, N, J, A, S>;

        static constexpr int dimension_T = T;
        static constexpr int dimension_L = L;
        static constexpr int dimension_M = M;
        static constexpr int dimension_I = I;
        static constexpr int dimension_O = O;
        static constexpr int dimension_N = N;
        static constexpr int dimension_J = J;
        static constexpr int dimension_A = A;
        static constexpr int dimension_S = S;

        static constexpr double DEFAULT_SCALE = 1.0;
        static inline const std::string DEFAULT_SYMBOL = "";

        template <int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2>
        Unit(const UnitResult<T2, L2, M2, I2, O2, N2, J2, A2, S2>& result) {
            static_assert(
                dimension_T == T2 &&
                dimension_L == L2 &&
                dimension_M == M2 &&
                dimension_I == I2 &&
                dimension_O == O2 &&
                dimension_N == N2 &&
                dimension_J == J2 &&
                dimension_A == A2 &&
                dimension_S == S2,
                "Cannot convert UnitResult to incompatible unit type"
                );

            value_ = result.getValue() * result.compute_scale() / scale_factor();
        }

        explicit Unit(double value = 0.0, const std::string& symbol_override = "");
        virtual ~Unit() = default;

        double getValue() const { return value_; }

        template <typename Derived = void>
        std::string getSymbol() const
        {
            return symbol();
        }
        virtual std::string getDerivedSymbol() const;

        // Addition operator overloads
        template <vira::IsUnit OtherUnit>
        Unit operator+=(const OtherUnit& other);
        template <vira::IsUnit OtherUnit>
        Unit operator+(const OtherUnit& other) const;


        // Subtraction operator overloads
        template <typename OtherUnit>
        Unit operator-=(const OtherUnit& other);
        template <vira::IsUnit OtherUnit>
        Unit operator-(const OtherUnit& other) const;


        // Multiplication operator overloads:
        template <vira::IsNumeric TNumeric>
        Unit& operator *= (TNumeric scalar);
        template <vira::IsNumeric TNumeric>
        Unit operator*(TNumeric scalar) const;
        template <int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2>
        auto operator*(const Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>& other) const;


        // Division operator overloads:
        template <vira::IsNumeric TNumeric>
        Unit& operator /= (TNumeric scalar);
        template <vira::IsNumeric TNumeric>
        Unit operator/(TNumeric scalar) const;
        template <int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2>
        auto operator/(const Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>& other) const;

    protected:
        double value_;
        std::string symbol_override_;

        // Default implementations
        static double scale_factor() { return DEFAULT_SCALE; }
        static std::string symbol() { return DEFAULT_SYMBOL; }

        // Allow ConcreteUnit to access internals
        template <typename D, typename B>
        friend class ConcreteUnit;
    };



    // =================================== //
    // === Dimension Extraction Helper === //
    // =================================== //
    template <typename UnitType>
    struct UnitDimensions {
        static constexpr int T = UnitType::dimension_T;
        static constexpr int L = UnitType::dimension_L;
        static constexpr int M = UnitType::dimension_M;
        static constexpr int I = UnitType::dimension_I;
        static constexpr int O = UnitType::dimension_O;
        static constexpr int N = UnitType::dimension_N;
        static constexpr int J = UnitType::dimension_J;
        static constexpr int A = UnitType::dimension_A;
        static constexpr int S = UnitType::dimension_S;
    };




    // ================================== //
    // === CRTP Concrete Units Helper === //
    // ================================== //
    template <typename Derived, typename BaseUnit>
    class ConcreteUnit : public BaseUnit {
    public:
        using base_unit_type = BaseUnit;

        explicit ConcreteUnit(double val = 0.0);

        // Copy Constructors
        template <IsNumeric TNumeric>
        ConcreteUnit(TNumeric val);
        ConcreteUnit(const ConcreteUnit& other);

        // Conversion constructor from any compatible unit
        template <IsUnit OtherUnit>
        explicit ConcreteUnit(const OtherUnit& other);

        // Add a conversion operator to allow implicit casting between units
        template <typename OtherUnit,
            typename = std::enable_if_t<std::is_base_of_v<typename OtherUnit::base_unit_type, BaseUnit> ||
            std::is_same_v<typename OtherUnit::base_unit_type, BaseUnit>>>
            operator OtherUnit() const {
            return OtherUnit(this->getValue() * Derived::scale_factor() / OtherUnit::scale_factor());
        }

        // Assignment operator for numeric types
        template <typename TNumeric,
            typename = std::enable_if_t<std::is_arithmetic_v<TNumeric>>>
        Derived& operator=(TNumeric val) {
            this->value = static_cast<double>(val);
            return static_cast<Derived&>(*this);
        }

        // Override the non-template method for symbol
        std::string getDerivedSymbol() const override {
            return Derived::symbol();
        }


        // Forward the operators to ensure correct type
        template <typename OtherUnit>
        Derived operator+(const OtherUnit& other) const;

        template <typename OtherUnit>
        Derived operator-(const OtherUnit& other) const;

        template <typename OtherUnit>
        auto operator*(const OtherUnit& other) const;
        Derived operator*(double scalar) const;

        template <typename OtherUnit>
        auto operator/(const OtherUnit& other) const;
        Derived operator/(double scalar) const;
    };
}

// Stream operator for Unit types
template <vira::IsUnit T>
std::ostream& operator<<(std::ostream& os, const T& input);

// Global scalar operators:
template <vira::IsNumeric T, vira::IsUnit UnitType>
auto operator/(T value, const UnitType& unit);

template <vira::IsNumeric T, vira::IsUnit UnitType>
auto operator*(T value, const UnitType& unit);

#include "implementation/units/base_unit.ipp"

#endif