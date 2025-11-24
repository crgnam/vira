#include <stdexcept>
#include <ostream>
#include <iomanip>
#include <string>
#include <type_traits>

namespace vira::units {
	// Primary Unit template
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	Unit<T, L, M, I, O, N, J, A, S>::Unit(double value, const std::string& symbol_override)
		: value_(value), symbol_override_(symbol_override)
	{

	};

	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	std::string Unit<T, L, M, I, O, N, J, A, S>::getDerivedSymbol() const {
		return !symbol_override_.empty() ? symbol_override_ : DEFAULT_SYMBOL;
	}


	// ========================== //
	// === Operator Overloads === //
	// ========================== //
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsUnit OtherUnit>
	Unit<T, L, M, I, O, N, J, A, S> Unit<T, L, M, I, O, N, J, A, S>::operator+=(const OtherUnit& other) {
		using ThisType = std::decay_t<decltype(*this)>;
		static_assert(std::is_same_v<unit_base_type, typename OtherUnit::unit_base_type>,
			"Units must have same dimensions for addition");

		// Get scale factors
		double this_scale = ThisType::scale_factor();
		double other_scale = OtherUnit::scale_factor();

		// Convert to this unit's scale
		double converted = other.getValue() * other_scale / this_scale;

		this->value_ += converted;

		return *this;
	};

	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsUnit OtherUnit>
	Unit<T, L, M, I, O, N, J, A, S> Unit<T, L, M, I, O, N, J, A, S>::operator+(const OtherUnit& other) const {
		Unit result = *this;
		result += other;
		return result;
	};


	// Subtraction operator overloads
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <typename OtherUnit>
	Unit<T, L, M, I, O, N, J, A, S> Unit<T, L, M, I, O, N, J, A, S>::operator-=(const OtherUnit& other) {
		using ThisType = std::decay_t<decltype(*this)>;
		static_assert(std::is_same_v<unit_base_type, typename OtherUnit::unit_base_type>,
			"Units must have same dimensions for subtraction");

		// Get scale factors
		double this_scale = ThisType::scale_factor();
		double other_scale = OtherUnit::scale_factor();

		// Convert to this unit's scale
		double converted = other.getValue() * other_scale / this_scale;

		this->value_ -= converted;

		return *this;
	};
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsUnit OtherUnit>
	Unit<T, L, M, I, O, N, J, A, S> Unit<T, L, M, I, O, N, J, A, S>::operator-(const OtherUnit& other) const {
		Unit result = *this;
		result -= other;
		return result;
	};


	// Multiplication operator overloads:
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsNumeric TNumeric>
	Unit<T, L, M, I, O, N, J, A, S>& Unit<T, L, M, I, O, N, J, A, S>::operator *= (TNumeric scalar) {
		this->value_ *= static_cast<double>(scalar);
		return *this;
	};
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsNumeric TNumeric>
	Unit<T, L, M, I, O, N, J, A, S> Unit<T, L, M, I, O, N, J, A, S>::operator*(TNumeric scalar) const {
		Unit output = *this;
		output *= scalar;
		return output;
	}
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2>
	auto Unit<T, L, M, I, O, N, J, A, S>::operator*(const Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>& other) const {
		double resultValue = value_ * other.getValue();

		// Get the scale factors
		double firstScale = 1.0;
		double secondScale = 1.0;

		using ThisType = std::decay_t<decltype(*this)>;
		using OtherType = std::decay_t<decltype(other)>;

		if constexpr (!std::is_same_v<ThisType, Unit>) {
			firstScale = ThisType::scale_factor();
		}

		if constexpr (!std::is_same_v<OtherType, Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>>) {
			secondScale = OtherType::scale_factor();
		}

		// Return a UnitResult for multiplication
		return UnitResult<T + T2, L + L2, M + M2, I + I2, O + O2, N + N2, J + J2, A + A2, S + S2>(
			resultValue,
			firstScale, secondScale,
			this->getDerivedSymbol(), other.getDerivedSymbol(),
			UnitResult<T + T2, L + L2, M + M2, I + I2, O + O2, N + N2, J + J2, A + A2, S + S2>::Operation::Multiply
		);
	}


	// Division operator overloads:
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsNumeric TNumeric>
	Unit<T, L, M, I, O, N, J, A, S>& Unit<T, L, M, I, O, N, J, A, S>::operator /= (TNumeric scalar) {
		this->value_ /= static_cast<double>(scalar);
		return *this;
	};
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <IsNumeric TNumeric>
	Unit<T, L, M, I, O, N, J, A, S> Unit<T, L, M, I, O, N, J, A, S>::operator/(TNumeric scalar) const {
		Unit output = *this;
		output /= scalar;
		return output;
	}
	template <int T, int L, int M, int I, int O, int N, int J, int A, int S>
	template <int T2, int L2, int M2, int I2, int O2, int N2, int J2, int A2, int S2>
	auto Unit<T, L, M, I, O, N, J, A, S>::operator/(const Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>& other) const {
		double resultValue = value_ / other.getValue();

		// Get the scale factors
		double numScale = 1.0;
		double denomScale = 1.0;

		using ThisType = std::decay_t<decltype(*this)>;
		using OtherType = std::decay_t<decltype(other)>;

		if constexpr (!std::is_same_v<ThisType, Unit>) {
			numScale = ThisType::scale_factor();
		}

		if constexpr (!std::is_same_v<OtherType, Unit<T2, L2, M2, I2, O2, N2, J2, A2, S2>>) {
			denomScale = OtherType::scale_factor();
		}

		// Return a UnitResult for division
		return UnitResult<T - T2, L - L2, M - M2, I - I2, O - O2, N - N2, J - J2, A - A2, S - S2>(
			resultValue, numScale, denomScale,
			this->getDerivedSymbol(), other.getDerivedSymbol(),
			UnitResult<T - T2, L - L2, M - M2, I - I2, O - O2, N - N2, J - J2, A - A2, S - S2>::Operation::Divide
		);
	};

	// Default constructor
	template <typename Derived, typename BaseUnit>
	ConcreteUnit<Derived, BaseUnit>::ConcreteUnit(double val) 
		: BaseUnit(val)
	{

	};

	// Non-explicit constructor from numeric types for direct assignment
	template <typename Derived, typename BaseUnit>
	template <IsNumeric TNumeric>
	ConcreteUnit<Derived, BaseUnit>::ConcreteUnit(TNumeric val) 
		: BaseUnit(static_cast<double>(val))
	{

	};

	// Copy constructor
	template <typename Derived, typename BaseUnit>
	ConcreteUnit<Derived, BaseUnit>::ConcreteUnit(const ConcreteUnit& other)
		: BaseUnit(other.getValue())
	{

	};

	// Conversion constructor from any compatible unit
	template <typename Derived, typename BaseUnit>
	template <IsUnit OtherUnit>
	ConcreteUnit<Derived, BaseUnit>::ConcreteUnit(const OtherUnit& other)
		: BaseUnit(other.getValue()* OtherUnit::scale_factor() / Derived::scale_factor())
	{

	};

	template <typename Derived, typename BaseUnit>
	template <typename OtherUnit>
	Derived ConcreteUnit<Derived, BaseUnit>::operator+(const OtherUnit& other) const {
		static_assert(std::is_same_v<base_unit_type, typename OtherUnit::base_unit_type>,
			"Units must have same dimensions");

		double other_value_in_this_units = other.getValue() *
			OtherUnit::scale_factor() /
			Derived::scale_factor();

		return Derived(this->getValue() + other_value_in_this_units);
	}

	template <typename Derived, typename BaseUnit>
	template <typename OtherUnit>
	Derived ConcreteUnit<Derived, BaseUnit>::operator-(const OtherUnit& other) const {
		static_assert(std::is_same_v<base_unit_type, typename OtherUnit::base_unit_type>,
			"Units must have same dimensions");

		double other_value_in_this_units = other.getValue() *
			OtherUnit::scale_factor() /
			Derived::scale_factor();

		return Derived(this->getValue() - other_value_in_this_units);
	}

	template <typename Derived, typename BaseUnit>
	template <typename OtherUnit>
	auto ConcreteUnit<Derived, BaseUnit>::operator*(const OtherUnit& other) const {
		// Use the helper template to extract dimensions
		using ThisDims = UnitDimensions<typename BaseUnit::unit_base_type>;
		using OtherDims = UnitDimensions<typename OtherUnit::unit_base_type>;

		// Calculate result value
		double resultValue = this->getValue() * other.getValue();

		// Return a result object with scale information preserved
		using ResultType = UnitResult<
			ThisDims::T + OtherDims::T,
			ThisDims::L + OtherDims::L,
			ThisDims::M + OtherDims::M,
			ThisDims::I + OtherDims::I,
			ThisDims::O + OtherDims::O,
			ThisDims::N + OtherDims::N,
			ThisDims::J + OtherDims::J,
			ThisDims::A + OtherDims::A,
			ThisDims::S + OtherDims::S
		>;

		return ResultType(
			resultValue,
			Derived::scale_factor(),
			OtherUnit::scale_factor(),
			Derived::symbol(),
			OtherUnit::symbol(),
			ResultType::Operation::Multiply
		);
	};

	template <typename Derived, typename BaseUnit>
	Derived ConcreteUnit<Derived, BaseUnit>::operator*(double scalar) const {
		return Derived(this->getValue() * scalar);
	}


	template <typename Derived, typename BaseUnit>
	template <typename OtherUnit>
	auto ConcreteUnit<Derived, BaseUnit>::operator/(const OtherUnit& other) const {
		// Use the helper template to extract dimensions
		using ThisDims = UnitDimensions<typename BaseUnit::unit_base_type>;
		using OtherDims = UnitDimensions<typename OtherUnit::unit_base_type>;

		// Calculate result value
		double resultValue = this->getValue() / other.getValue();

		// Return a result object with scale information preserved
		using ResultType = UnitResult<
			ThisDims::T - OtherDims::T,
			ThisDims::L - OtherDims::L,
			ThisDims::M - OtherDims::M,
			ThisDims::I - OtherDims::I,
			ThisDims::O - OtherDims::O,
			ThisDims::N - OtherDims::N,
			ThisDims::J - OtherDims::J,
			ThisDims::A - OtherDims::A,
			ThisDims::S - OtherDims::S
		>;

		return ResultType(
			resultValue,
			Derived::scale_factor(),
			OtherUnit::scale_factor(),
			Derived::symbol(),
			OtherUnit::symbol(),
			ResultType::Operation::Divide
		);
	};

	template <typename Derived, typename BaseUnit>
	Derived ConcreteUnit<Derived, BaseUnit>::operator/(double scalar) const {
		return Derived(this->getValue() / scalar);
	};

	using ScalarUnit = Unit<0, 0, 0, 0, 0, 0, 0, 0, 0>;
	template <typename T>
	concept IsScalarUnit = DerivedFrom<T, ScalarUnit> || IsNumeric<T>;
};

// Stream operator for Unit types
template <vira::IsUnit T>
std::ostream& operator<<(std::ostream& os, const T& input)
{
	std::ios oldState(nullptr);
	oldState.copyfmt(os);

	os << std::fixed << std::setprecision(6);
	os << input.getValue() << " " << input.getDerivedSymbol();

	os.copyfmt(oldState);
	return os;
};

// Global scalar operators:
template <vira::IsNumeric T, vira::IsUnit UnitType>
auto operator/(T value, const UnitType& unit)
{
	return vira::units::ScalarUnit(static_cast<double>(value)) / unit;
};

template <vira::IsNumeric T, vira::IsUnit UnitType>
auto operator*(T value, const UnitType& unit)
{
	return vira::units::ScalarUnit(static_cast<double>(value)) * unit;
};