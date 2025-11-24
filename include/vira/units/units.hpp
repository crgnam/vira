#ifndef VIRA_UNITS_UNITS_HPP
#define VIRA_UNITS_UNITS_HPP

#include <string>

#include "vira/math.hpp"
#include "vira/units/prefixes.hpp"
#include "vira/units/unit_types.hpp"

namespace vira::units {
	// ================== //
    // === Time Units === //
    // ================== //
	class Second : public ConcreteUnit<Second, TimeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "s"; }
	};

	using Nanosecond = PrefixedUnit<Second, std::nano>;
	using Microsecond = PrefixedUnit<Second, std::micro>;
	using Millisecond = PrefixedUnit<Second, std::milli>;

	class Minute : public ConcreteUnit<Minute, TimeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 60.0; }
		static std::string symbol() { return "min"; }
	};

	class Hour : public ConcreteUnit<Hour, TimeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 3600.0; }
		static std::string symbol() { return "hr"; }
	};

	class Day : public ConcreteUnit<Day, TimeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 86400.0; }
		static std::string symbol() { return "d"; }
	};
	using SolarDay = Day;

	class SiderealDay : public ConcreteUnit<SiderealDay, TimeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 86164.1; }
		static std::string symbol() { return "sidereal-day"; }
	};

	class JulianYear : public ConcreteUnit<JulianYear, TimeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 31557600.0; }
		static std::string symbol() { return "jy"; }
	};



	// ==================== //
	// === Length Units === //
	// ==================== //
	class Meter : public ConcreteUnit<Meter, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "m"; }
	};

	using Nanometer = PrefixedUnit<Meter, std::nano>;
	using Micrometer = PrefixedUnit<Meter, std::micro>;
	using Millimeter = PrefixedUnit<Meter, std::milli>;
	using Centimeter = PrefixedUnit<Meter, std::centi>;
	using Kilometer = PrefixedUnit<Meter, std::kilo>;


	class Inch : public ConcreteUnit<Inch, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 0.0254; }
		static std::string symbol() { return "in"; }
	};

	class Foot : public ConcreteUnit<Foot, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 0.3048; }
		static std::string symbol() { return "ft"; }
	};

	class Yard : public ConcreteUnit<Yard, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 0.9144; }
		static std::string symbol() { return "yd"; }
	};

	class Mile : public ConcreteUnit<Mile, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1609.344; }
		static std::string symbol() { return "mi"; }
	};


	class AstronomicalUnit : public ConcreteUnit<AstronomicalUnit, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 149597870700.0; }
		static std::string symbol() { return "AU"; }
	};

	class Lightyear : public ConcreteUnit<Lightyear, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 9460730472580.8; }
		static std::string symbol() { return "ly"; }
	};

	class Parsec : public ConcreteUnit<Parsec, LengthUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return (648000.0 / PI<double>()) * 149597870700.0; }
		static std::string symbol() { return "pc"; }
	};
	using Megaparsec = PrefixedUnit<Parsec, std::mega>;
	using Gigaparsec = PrefixedUnit<Parsec, std::giga>;



	// ================== //
	// === Mass Units === //
	// ================== //
	class Gram : public ConcreteUnit<Gram, MassUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1e-3; }
		static std::string symbol() { return "g"; }
	};
	using Microgram = PrefixedUnit<Gram, std::micro>;
	using Milligram = PrefixedUnit<Gram, std::milli>;
	using Kilogram = PrefixedUnit<Gram, std::kilo>;



	// ===================== //
	// === Current Units === //
	// ===================== //
	class Ampere : public ConcreteUnit<Ampere, CurrentUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "A"; }
	};
	using MicroAmpere = PrefixedUnit<Ampere, std::micro>;
	using MilliAmpere = PrefixedUnit<Ampere, std::milli>;
	using KiloAmpere = PrefixedUnit<Ampere, std::kilo>;


	// ========================= //
	// === Temperature Units === // 
	// ========================= //
	class Kelvin : public ConcreteUnit<Kelvin, TemperatureUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "K"; }
	};

	class Celsius : public ConcreteUnit<Celsius, TemperatureUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "C"; }
	};



	// ================================= //
	// === Amount Of Substance Units === //
	// ================================= //
	class Mole : public ConcreteUnit<Mole, AmountOfSubstanceUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "mol"; }
	};



	// =============================== //
	// === Luminous Intensity Unit === //
	// =============================== //
	class Candela : public ConcreteUnit<Candela, LuminousIntensityUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "cd"; }
	};


	// =================== //
	// === Angle Units === //
	// =================== //
	class Radian : public ConcreteUnit<Radian, AngleUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "rad"; }
	};
	using Nanoradian = PrefixedUnit<Radian, std::nano>;
	using Microradian = PrefixedUnit<Radian, std::micro>;
	using Milliradian = PrefixedUnit<Radian, std::milli>;

	class Degree : public ConcreteUnit<Degree, AngleUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return PI<double>() / 180.0; }
		static std::string symbol() { return "deg"; }
	};

	class Steradian : public ConcreteUnit<Steradian, SolidAngleUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "sr"; }
	};
	using Nanosteradian = PrefixedUnit<Steradian, std::nano>;
	using Microsteradian = PrefixedUnit<Steradian, std::micro>;
	using Millisteradian = PrefixedUnit<Steradian, std::milli>;



	
	// ===================== //
	// === Derived Units === //
	// ===================== //
	class MetersSquared : public ConcreteUnit<MetersSquared, AreaUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "m^2"; }
	};

	class MetersPerSecond : public ConcreteUnit<MetersPerSecond, VelocityUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "m/s"; }
	};

	class KilometersPerSecond : public ConcreteUnit<KilometersPerSecond, VelocityUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1000.0; }
		static std::string symbol() { return "km/s"; }
	};

	class MilesPerHour : public ConcreteUnit<MilesPerHour, VelocityUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 0.44704; }
		static std::string symbol() { return "mph"; }
	};

	class Watt : public ConcreteUnit<Watt, PowerUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "W"; }
	};

	using Kilowatt = PrefixedUnit<Watt, std::kilo>;
	using Megawatt = PrefixedUnit<Watt, std::mega>;
	using Gigawatt = PrefixedUnit<Watt, std::giga>;

	class Joule : public ConcreteUnit<Joule, EnergyUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "J"; }
	};
	using Kilojoule = PrefixedUnit<Joule, std::kilo>;
	using Megajoule = PrefixedUnit<Joule, std::mega>;
	using Gigajoule = PrefixedUnit<Joule, std::giga>;


	class ElectronVolt : public ConcreteUnit<ElectronVolt, EnergyUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.602176634e-19; }
		static std::string symbol() { return "eV"; }
	};


	class Hertz : public ConcreteUnit<Hertz, FrequencyUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "Hz"; }
	};
	using Kilohertz = PrefixedUnit<Hertz, std::kilo>;
	using Megahertz = PrefixedUnit<Hertz, std::mega>;
	using Gigahertz = PrefixedUnit<Hertz, std::giga>;
	using Terahertz = PrefixedUnit<Hertz, std::tera>;


	class Coulomb : public ConcreteUnit<Coulomb, ChargeUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "C"; }
	};

	class Radiance : public ConcreteUnit<Radiance, RadianceUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "Le"; }
	};

	class Irradiance : public ConcreteUnit<Irradiance, IrradianceUnit> {
	public:
		using ConcreteUnit::ConcreteUnit;

		static double scale_factor() { return 1.0; }
		static std::string symbol() { return "Ee"; }
	};
};

#endif