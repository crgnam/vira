#ifndef VIRA_CONSTRAINTS_HPP
#define VIRA_CONSTRAINTS_HPP

#include <concepts>
#include <cstdint>
#include <string>

namespace vira {
    /**
     * @brief Variadic helper concept to compare a type against any from a set of of types
     * @details
     * ```cpp
     * template <typename T, typename... U>
     * concept IsAnyOf = (std::same_as<T, U> || ...);
     * ```
     */
    template <typename T, typename... U>
    concept IsAnyOf = (std::same_as<T, U> || ...);


    /**
     * @brief Concept to ensure type is derived from a class template
     * @details
     * ```cpp
     * template<typename Derived, template<typename...> typename Base>
     * concept derived_from_template = requires (Derived & d) {
     *     [] <typename... Ts>(Base<Ts...>&) {}(d);
     * };
     * ```
     */
    template<typename Derived, template<typename...> typename Base>
    concept DerivedFromTemplate = requires (Derived & d) {
        [] <typename... Ts>(Base<Ts...>&) {}(d);
    };

    template <typename Derived, typename Base>
    concept DerivedFrom = std::is_base_of_v<Base, Derived>;

    // Primary template
    template <typename, template <typename...> class>
    struct is_specialization_of : std::false_type {};

    // Match type-based templates
    template <template <typename...> class Template, typename... Args>
    struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

    // Concept that uses the metafunction
    template <typename T, template <typename...> class Template>
    concept IsSpecializationOf = is_specialization_of<T, Template>::value;

    // ======================== //
    // === Unit Constraints === //
    // ======================== //
    // Type trait to detect if a type is any specialization of Unit
    template <typename T>
    struct is_unit_specialization : std::false_type {};

    // Forward Declare:
    namespace units {
        template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
        class Unit;
    }

    template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
    struct is_unit_specialization<units::Unit<T, L, M, I, O, N, J, A, S>> : std::true_type {};

    // Concept for a direct Unit specialization
    template <typename T>
    concept UnitSpecialization = is_unit_specialization<T>::value;

    // Concept for a type that is either a Unit specialization or derived from one
    template <typename T>
    concept IsUnit = requires {
        // Check for typical members that exist in Unit but not in UnitResult
        typename T::unit_base_type;
        // Check for the dimension constants that exist in Unit
        { T::dimension_T } -> std::convertible_to<int>;
        { T::dimension_L } -> std::convertible_to<int>;
        { T::dimension_M } -> std::convertible_to<int>;
        { T::dimension_I } -> std::convertible_to<int>;
        { T::dimension_O } -> std::convertible_to<int>;
        { T::dimension_N } -> std::convertible_to<int>;
        { T::dimension_J } -> std::convertible_to<int>;
        { T::dimension_A } -> std::convertible_to<int>;
        { T::dimension_S } -> std::convertible_to<int>;
        // Check for the scale_factor method with no arguments
        { std::declval<T>().scale_factor() } -> std::convertible_to<double>;

        // Specific requirement to exclude UnitResult
        // UnitResult has compute_scale method but Unit doesn't
        !requires(T t) { t.compute_scale(); };

        // UnitResult has Operation enum class
        !requires { typename T::Operation; };
    };

    template <typename T>
    concept IsUnitResult = requires {
        // Check for typical members that exist in UnitResult
        typename T::unit_base_type;
        typename T::dimension_type;
        typename T::Operation; // Check for the Operation enum

        // Check for methods specific to UnitResult
        { std::declval<T>().compute_scale() } -> std::convertible_to<double>;
        { std::declval<T>().getSymbol() } -> std::convertible_to<std::string>;
        { std::declval<T>().getValue() } -> std::convertible_to<double>;

        // Specifically exclude Unit-only characteristics
        !requires { T::dimension_T; };
        !requires { T::DEFAULT_SCALE; };
        !requires { T::DEFAULT_SYMBOL; };
    }&& requires(double value, double scale1, double scale2,
        const std::string& symbol1, const std::string& symbol2) {
        // Check constructor syntax separately to avoid the error
        T(value, scale1, scale2, symbol1, symbol2);
    };

    /**
     * @brief Represents an boolean concept
     * @details
     * ```cpp
     * template <typename T>
     * concept IsBool = std::same_as<T, bool>;
     * ```
     */
    template <typename T>
    concept IsBool = std::same_as<T, bool>;


    /**
     * @brief Represents an integer concept
     * @details
     * ```cpp
     * template <typename T>
     * concept IsInteger = std::is_integral_v<T>;
     * ```
     */
    template <typename T>
    concept IsInteger = std::is_integral_v<T>;


    /**
     * @brief Represents a floating point concept (`float` or `double)
     * @details
     * ```cpp
     * template <typename T>
     * concept IsFloat = std::floating_point<T>;
     * ```
     */
    template <typename T>
    concept IsFloat = std::is_floating_point_v<T>;


    /**
     * @brief Represents a Numeric concept
     * @details
     * ```cpp
     * template <typename T>
     * concept IsNumeric = IsInteger<T> || IsFloat<T>;
     * ```
    */
    template <typename T>
    concept IsNumeric = IsInteger<T> || IsFloat<T>;


    /**
     * @brief Represents a lesser precision floating-point constraint
     * @details This constraint requires that, for two floating point types, the second must be of equal or lesser precision.
     * ```cpp
     * template <typename T, typename T2>
     * concept LesserFloat = requires
     * {
     *     requires (std::same_as<T, float> and std::same_as<T2, float>) or (std::same_as<T, double> and IsFloat<T2>);
     * };
     * ```
    */
    template <typename T, typename T2>
    concept LesserFloat = requires
    {
        requires (std::same_as<T, float> and std::same_as<T2, float>) or (std::same_as<T, double> and IsFloat<T2>);
    };


    // TODO use this rather than the requires clause:
    template <typename TMeshFloat, typename TFloat>
    concept IsMeshFloat = requires
    {
        requires IsFloat<TMeshFloat>&& IsFloat<TFloat>;
        requires std::numeric_limits<TMeshFloat>::digits <= std::numeric_limits<TFloat>::digits;
    };


    /**
     * @brief Concept to represent having basic arithmetic operator overloads with the same type
     *
     * ```cpp
     * template <typename T>
     * concept HasSelfMath = requires(T a, T b) {
     *     { a + b } -> std::same_as<T>;
     *     { a* b } -> std::same_as<T>;
     *     { a - b } -> std::same_as<T>;
     * };
     * ```
     */
    template <typename T>
    concept HasSelfMath = requires(T a, T b) {
        { a + b } -> std::same_as<T>;
        { a* b } -> std::same_as<T>;
        { a - b } -> std::same_as<T>;
    };

    /**
     * @brief Concept to represent having basic arithmetic operator overloads with floating point types
     * @details
     * ```cpp
     * template <typename T, typename TFloat>
     * concept HasFloatMath = requires(T a, TFloat b) {
     *     { a + b } -> std::same_as<T>;
     *     { a* b } -> std::same_as<T>;
     *     { a - b } -> std::same_as<T>;
     *
     *     { b + a } -> std::same_as<T>;
     *     { b* a } -> std::same_as<T>;
     *     { b - a } -> std::same_as<T>;
     * };
     * ```
     */
    template <typename T, typename TFloat>
    concept HasFloatMath = requires(T a, TFloat b) {
        { a + b } -> std::same_as<T>;
        { a* b } -> std::same_as<T>;
        { a - b } -> std::same_as<T>;

        { b + a } -> std::same_as<T>;
        { b* a } -> std::same_as<T>;
        { b - a } -> std::same_as<T>;
    };


    /**
     * @brief Concept to represent having basic arithmetic operator overloads
     * @details
     * ```cpp
     * template <typename T, typename TFloat>
     * concept HasMath = requires {
     *     requires (HasFloatMath<T, TFloat> and HasSelfMath<T>);
     * };
     * ```
     */
    template <typename T, typename TFloat>
    concept HasMath = requires {
        requires (HasFloatMath<T, TFloat> and HasSelfMath<T>);
    };

    template <IsNumeric T>
    std::string NUMERIC_STRING() {
        std::string str;
        if constexpr (std::same_as<T, float>) {
            str = "float";
        }
        else if constexpr (std::same_as<T, double>) {
            str = "double";
        }
        else if constexpr (std::same_as<T, uint8_t>) {
            str = "uint8_t";
        }
        else if constexpr (std::same_as<T, uint16_t>) {
            str = "uint16_t";
        }
        else if constexpr (std::same_as<T, uint32_t>) {
            str = "uint32_t";
        }
        else if constexpr (std::same_as<T, uint64_t>) {
            str = "uint64_t";
        }
        else if constexpr (std::same_as<T, int8_t>) {
            str = "int8_t";
        }
        else if constexpr (std::same_as<T, int16_t>) {
            str = "int16_t";
        }
        else if constexpr (std::same_as<T, int32_t>) {
            str = "int32_t";
        }
        else if constexpr (std::same_as<T, int64_t>) {
            str = "int64_t";
        }
        else {
            str = "NUMERIC";
        }

        return str;
    };

    // Helper to check numeric interoperability
    template <typename T, typename N>
    concept HasNumericInterop = requires(T a, N n) {
        { a + n } -> std::convertible_to<T>;
        { n + a } -> std::convertible_to<T>;
        { a - n } -> std::convertible_to<T>;
        { n - a } -> std::convertible_to<T>;
        { a* n } -> std::convertible_to<T>;
        { n* a } -> std::convertible_to<T>;
        { a / n } -> std::convertible_to<T>;
        { n / a } -> std::convertible_to<T>;

        { a += n } -> std::same_as<T&>;
        { a -= n } -> std::same_as<T&>;
        { a *= n } -> std::same_as<T&>;
        { a /= n } -> std::same_as<T&>;
    };

    // Base concept for IsCustomNumeric
    template <typename T>
    concept IsCustomNumeric = requires(T a, T b) {
        // Binary arithmetic operators with same type
        { a + b } -> std::convertible_to<T>;
        { a - b } -> std::convertible_to<T>;
        { a* b } -> std::convertible_to<T>;
        { a / b } -> std::convertible_to<T>;

        // Compound assignment operators
        { a += b } -> std::same_as<T&>;
        { a -= b } -> std::same_as<T&>;
        { a *= b } -> std::same_as<T&>;
        { a /= b } -> std::same_as<T&>;

        // Comparison operators
        { a == b } -> std::convertible_to<bool>;
        { a != b } -> std::convertible_to<bool>;
        { a < b } -> std::convertible_to<bool>;
        { a <= b } -> std::convertible_to<bool>;
        { a > b } -> std::convertible_to<bool>;
        { a >= b } -> std::convertible_to<bool>;
    }&&
        // Interoperability with all standard numeric types
        HasNumericInterop<T, int>&&
        HasNumericInterop<T, unsigned int>&&
        HasNumericInterop<T, long>&&
        HasNumericInterop<T, unsigned long>&&
        HasNumericInterop<T, long long>&&
        HasNumericInterop<T, unsigned long long>&&
        HasNumericInterop<T, float>&&
        HasNumericInterop<T, double>&&
        HasNumericInterop<T, long double>&&
        HasNumericInterop<T, int8_t>&&
        HasNumericInterop<T, uint8_t>&&
        HasNumericInterop<T, int16_t>&&
        HasNumericInterop<T, uint16_t>&&
        HasNumericInterop<T, int32_t>&&
        HasNumericInterop<T, uint32_t>&&
        HasNumericInterop<T, int64_t>&&
        HasNumericInterop<T, uint64_t>;


    // Spectral Conversion template arguments:
    template<typename TConverter, typename TInputSpectral, typename TOutputSpectral>
    concept IsSpectralConverter = requires(TConverter converter, const TInputSpectral & input) {
        { converter(input) } -> std::convertible_to<TOutputSpectral>;
    };
};

#endif