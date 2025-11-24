#ifndef VIRA_MATH_HPP
#define VIRA_MATH_HPP

#include <array>
#include <cstddef>
#include <vector>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"

namespace vira {
    // Math constants:
    template <IsFloat T>
    inline constexpr T PI() { return static_cast<T>(3.141592653589793238462); };

    template <IsFloat T>
    inline constexpr T INV_PI() { return static_cast<T>(1. / 3.141592653589793238462); };

    template <IsFloat T>
    inline constexpr T PI_OVER_2() { return PI<T>() / T{ 2 }; };

    template <IsFloat T>
    inline constexpr T PI_OVER_4() { return PI<T>() / T{ 4 }; };

    template <IsFloat T>
    inline constexpr T INV_2_PI() { return T{ 1 } / (T{ 2 } * PI<T>()); };

    template <IsFloat T>
    inline constexpr T INV_4_PI() { return T{ 1 } / (T{ 4 } * PI<T>()); };

    template <IsFloat T>
    inline constexpr T SPEED_OF_LIGHT() { return static_cast<T>(299792458); }; // m/s

    template <IsFloat T>
    inline constexpr T PLANCK_CONSTANT() { return static_cast<T>(6.62607015e-34); }; // J/Hz

    template <IsFloat T>
    inline constexpr T BOLTZMANN_CONSTANT() { return static_cast<T>(1.380649e-23); }; // J/K

    template <IsFloat T>
    inline constexpr T RAD2DEG() { return T{ 180 } / PI<T>(); }

    template <IsFloat T>
    inline constexpr T DEG2RAD() { return PI<T>() / T{ 180 }; }

    template <IsFloat T>
    inline constexpr T NANOMETERS() { return static_cast<T>(1e-9); }

    template <IsFloat T>
    inline constexpr T SECONDS_PER_DAY() { return T{ 86400 }; }

    template <IsFloat T>
    inline constexpr T SECONDS_PER_YEAR() { return T{ 365.25 } * SECONDS_PER_DAY<T>(); }

    inline constexpr float SOLAR_RADIUS() { return 696000000; }

    template <IsFloat T>
    inline constexpr T INF() { return std::numeric_limits<T>::infinity(); }

    template <IsFloat T>
    inline constexpr T AUtoKM() { return static_cast<T>(149597870.691); }

    template<IsFloat T>
    inline constexpr T AUtoM() { return static_cast<T>(149597870700); }


    template <IsFloat T>
    constexpr T PhotonEnergy(T wavelength);

    template <IsFloat T>
    constexpr T PhotonEnergyFreq(T wavelength);

    template <IsFloat T>
    std::vector<T> linspace(T min, T max, size_t N);

    template <IsFloat T, size_t N>
    constexpr std::array<T, N> linspaceArray(T min, T max);

    template <IsFloat T>
    T plancksLaw(T temperature, T wavelength);

    template <IsFloat T>
    std::vector<T> plancksLaw(T temperature, std::vector<T> wavelengths);

    template <IsFloat T>
    T plancksLawFreq(T temperature, T frequency);

    template <IsFloat T>
    std::vector<T> plancksLawFreq(T temperature, std::vector<T> frequencies);

    template <IsFloat T>
    T linearInterpolate(T sampleX, const std::vector<T>& x, const std::vector<T>& y);

    template <IsFloat T>
    T trapezoidIntegrate(const std::vector<T>& x, const std::vector<T>& y, T bmin = -INF<T>(), T bmax = INF<T>());

    template <IsFloat T>
    T vira_cyl_bessel_j(double nu, double x);
};

#include "implementation/math.ipp"

#endif
