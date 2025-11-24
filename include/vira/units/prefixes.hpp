#ifndef VIRA_UNITS_PREFIXES_HPP
#define VIRA_UNITS_PREFIXES_HPP

#include <string>
#include <ratio>

#include "vira/constraints.hpp"
#include "vira/units/base_unit.hpp"

namespace vira::units {
    template <typename>
    struct is_ratio : std::false_type {};

    template <intmax_t Num, intmax_t Den>
    struct is_ratio<std::ratio<Num, Den>> : std::true_type {};

    template <typename T>
    concept IsRatio = is_ratio<T>::value;

    // Define standard SI prefixes
    template<typename Ratio>
    struct PrefixSymbol {
        static std::string value() { return ""; } // default to empty for unknowns
    };

    // Specializations for known prefixes
    template<> struct PrefixSymbol<std::atto>  { static std::string value() { return "a";  } };
    template<> struct PrefixSymbol<std::femto> { static std::string value() { return "f";  } };
    template<> struct PrefixSymbol<std::pico>  { static std::string value() { return "p";  } };
    template<> struct PrefixSymbol<std::nano>  { static std::string value() { return "n";  } };
    template<> struct PrefixSymbol<std::micro> { static std::string value() { return "u";  } };
    template<> struct PrefixSymbol<std::milli> { static std::string value() { return "m";  } };
    template<> struct PrefixSymbol<std::centi> { static std::string value() { return "c";  } };
    template<> struct PrefixSymbol<std::deci>  { static std::string value() { return "d";  } };
    template<> struct PrefixSymbol<std::deca>  { static std::string value() { return "da"; } };
    template<> struct PrefixSymbol<std::hecto> { static std::string value() { return "h";  } };
    template<> struct PrefixSymbol<std::kilo>  { static std::string value() { return "k";  } };
    template<> struct PrefixSymbol<std::mega>  { static std::string value() { return "M";  } };
    template<> struct PrefixSymbol<std::giga>  { static std::string value() { return "G";  } };
    template<> struct PrefixSymbol<std::tera>  { static std::string value() { return "T";  } };
    template<> struct PrefixSymbol<std::peta>  { static std::string value() { return "P";  } };
    template<> struct PrefixSymbol<std::exa>   { static std::string value() { return "E";  } };


    // Prefixed unit template generator - now using only scale factor as template parameter
    template <IsUnit BaseUnit, typename ScaleRatio>
    class PrefixedUnit : public ConcreteUnit<PrefixedUnit<BaseUnit, ScaleRatio>, typename BaseUnit::base_unit_type> {
    public:
        // Default constructor
        PrefixedUnit() : ConcreteUnit<PrefixedUnit<BaseUnit, ScaleRatio>, typename BaseUnit::base_unit_type>() {}

        // Value constructor
        explicit PrefixedUnit(double val)
            : ConcreteUnit<PrefixedUnit<BaseUnit, ScaleRatio>, typename BaseUnit::base_unit_type>(val) {}

        // Numeric type constructor (for direct assignment)
        template <IsNumeric NumericType>
        PrefixedUnit(NumericType val)
            : ConcreteUnit<PrefixedUnit<BaseUnit, ScaleRatio>, typename BaseUnit::base_unit_type>(static_cast<double>(val)) {}

        // Copy constructor
        PrefixedUnit(const PrefixedUnit& other)
            : ConcreteUnit<PrefixedUnit<BaseUnit, ScaleRatio>, typename BaseUnit::base_unit_type>(other.getValue()) {}

        // Conversion constructor
        template <IsUnit OtherUnit>
        explicit PrefixedUnit(const OtherUnit& other)
            : ConcreteUnit<PrefixedUnit<BaseUnit, ScaleRatio>, typename BaseUnit::base_unit_type>(
                other.getValue()* OtherUnit::scale_factor() / scale_factor()) {
        }

        // Scale factor is base unit's scale multiplied by the template parameter
        static double scale_factor() {
            return BaseUnit::scale_factor() * static_cast<double>(ScaleRatio::num) / ScaleRatio::den;
        }

        // Symbol is prefix symbol + base unit symbol
        static std::string symbol() {
            return PrefixSymbol<ScaleRatio>::value() + BaseUnit::symbol();
        }
    };
};

#endif