#include <vector>
#include <array>
#include <cmath>
#include <stdexcept>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"

namespace vira {
    template <IsFloat T>
    constexpr T PhotonEnergy(T wavelength)
    {
        return PLANCK_CONSTANT<T>() * SPEED_OF_LIGHT<T>() / wavelength;
    };

    template <IsFloat T>
    constexpr T PhotonEnergyFreq(T frequency)
    {
        return PLANCK_CONSTANT<T>() * frequency;
    };

    template <IsFloat T, size_t N>
    constexpr std::array<T, N> linspaceArray(T min, T max)
    {
        std::array<T, N> array;
        T step = (max - min) / static_cast<T>(N - 1);
        for (size_t i = 0; i < N; ++i) {
            array[i] = min + (static_cast<T>(i) * step);
        }
        return array;
    };

    template <IsFloat T>
    std::vector<T> linspace(T min, T max, size_t N)
    {
        std::vector<T> output(N);
        T step = (max - min) / static_cast<T>(N - 1);
        for (size_t i = 0; i < N; ++i) {
            output[i] = min + (static_cast<T>(i) * step);
        }
        return output;
    };


    template <IsFloat T>
    T plancksLaw(T temperature, T wavelength)
    {
        // Use double-precision for numerical accuracy:
        constexpr double c = SPEED_OF_LIGHT<double>();
        constexpr double h = PLANCK_CONSTANT<double>();
        constexpr double kB = BOLTZMANN_CONSTANT<double>();
        double lambda = static_cast<double>(wavelength);
        double l5 = std::pow(lambda, 5.);

        double coeff = 2 * h * c * c / l5;
        double spectralRadiance = coeff * 1. / (std::exp(h * c / (lambda * kB * temperature)) - 1.);

        return static_cast<T>(spectralRadiance);
    };

    template <IsFloat T>
    std::vector<T> plancksLaw(T temperature, std::vector<T> wavelengths)
    {
        std::vector<T> rad(wavelengths.size());
        for (size_t i = 0; i < wavelengths.size(); ++i) {
            rad[i] = plancksLaw(temperature, wavelengths[i]);
        }
        return rad;
    };

    template <IsFloat T>
    T plancksLawFreq(T temperature, T frequency)
    {
        // Use double-precision for numerical accuracy:
        constexpr double c = SPEED_OF_LIGHT<double>();
        constexpr double h = PLANCK_CONSTANT<double>();
        constexpr double kB = BOLTZMANN_CONSTANT<double>();
        double nu = static_cast<double>(frequency);
        double nu3 = std::pow(nu, 3.);

        double coeff = 2 * h * nu3 / (c * c);
        double spectralRadiance = coeff * 1. / (std::exp(h * nu / (kB * temperature)) - 1.);

        return static_cast<T>(spectralRadiance);
    };

    template <IsFloat T>
    std::vector<T> plancksLawFreq(T temperature, std::vector<T> frequencies)
    {
        std::vector<T> rad(frequencies.size());
        for (size_t i = 0; i < frequencies.size(); ++i) {
            rad[i] = plancksLawFreq(temperature, frequencies[i]);
        }
        return rad;
    };

    template <IsFloat T>
    T linearInterpolate(T sampleX, const std::vector<T>& x, const std::vector<T>& y)
    {
        // Input must be sorted (X must be monotonically increasing)
        if (x.size() != y.size() || x.size() < 2) {
            throw std::runtime_error("linearInterpolate requires x.size() == y.size() >= 2");
        }
        size_t N = x.size();

        size_t i = 0;
        if (sampleX > x[N - 1]) {
            // Extrapolation above the range:
            i = N - 2;
        }
        else if (sampleX < y[0]) {
            // Extrapolation below the range:
            i = 0;
        }
        else {
            // Find the interval containing targetX
            while (i < (N - 1) && x[i + 1] < sampleX) {
                ++i;
            }
        }

        // Linear interpolation formula
        T x0 = x[i];
        T y0 = y[i];
        T x1 = x[i + 1];
        T y1 = y[i + 1];

        return y0 + (sampleX - x0) * (y1 - y0) / (x1 - x0);
    }

    template <IsFloat T>
    T trapezoidIntegrate(const std::vector<T>& x, const std::vector<T>& y, T bmin, T bmax)
    {
        // Input must be sorted (X must be monotonically increasing)
        if (x.size() != y.size() || x.size() < 2) {
            throw std::runtime_error("trapezoidIntegrate requires x.size() == y.size() >= 2");
        }
        size_t N = x.size();

        // Limit the bounds:
        bmin = std::max(bmin, x[0]);
        bmax = std::min(bmax, x[N - 1]);

        // Compute integral:
        double area = 0;
        for (size_t i = 0; i < (N - 1); ++i) {

            // Check if there is no overlap between current bin and integration domain:
            if (x[i + 1] < bmin || x[i] > bmax) {
                continue;
            }

            // Interpolate left edge:
            T b1 = std::max(bmin, x[i]);
            T y1 = y[i];
            if (b1 != x[i]) {
                y1 = linearInterpolate(b1, x, y);
            }

            // Interpolate right edge:
            T b2 = std::min(bmax, x[i + 1]);
            T y2 = y[i + 1];
            if (b2 != x[i + 1]) {
                y2 = linearInterpolate(b2, x, y);
            }

            // Compute trapezoid area:
            double dx = static_cast<double>(b2 - b1);
            area += dx * static_cast<double>(y1 + y2) / 2.;
        }

        return static_cast<T>(area);
    };


    // Custom implementation of bessel function:
    template <IsFloat T>
    T vira_cyl_bessel_j(double nu, double x)
    {
#ifdef __cpp_lib_math_special_functions
        return static_cast<T>(std::cyl_bessel_j(nu, x));
#else
    // TODO Check if this is valid:
        const int MAXITER = 1000;
        const double EPSILON = 1e-15;

        T sum = static_cast<T>(std::pow(x / 2, nu) / std::tgamma(nu + 1));
        T term = sum;  // First term

        for (int k = 1; k <= MAXITER; ++k) {
            term *= -static_cast<T>(std::pow(x / 2, 2) / (k * (k + nu)));
            sum += term;

            if (std::abs(term) <= EPSILON * std::abs(sum)) {
                break;
            }
        }

        return sum;
#endif
    };
};