#include <array>
#include <cstdint>
#include <cmath>
#include <string>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

#include "vira/math.hpp"

namespace fs = std::filesystem;

namespace vira {
    // ======================================= //
    // === Helper Initialization Functions === //
    // ======================================= //    
    template <auto T1, auto T2>
    constexpr bool ValidBoundsPair()
    {
        return static_cast<float>(T1) < static_cast<float>(T2);
    }

    template <auto T1, auto T2, auto... Rest>
    constexpr bool ValidBoundsPairs()
    {
        if constexpr (sizeof...(Rest) == 0) {
            return ValidBoundsPair<T1, T2>();
        }
        else {
            return ValidBoundsPair<T1, T2>() && ValidBoundsPairs<Rest...>();
        }
    }

    template <auto T1, auto T2>
    constexpr bool ValidBandwidthPairs()
    {
        bool valid = true;
        float center = static_cast<float>(T1);
        float bandwidth = static_cast<float>(T2);
        valid = (valid && (center > 0));
        valid = (valid && (bandwidth > 0));
        valid = (valid && ((center - bandwidth) > 0));

        return valid;
    };

    template <auto T1, auto T2, auto... Rest>
    constexpr bool ValidBandwidthPairs()
    {
        if constexpr (sizeof...(Rest) == 0) {
            return ValidBandwidthPairs<T1, T2>();
        }
        else {
            return ValidBandwidthPairs<T1, T2>() && ValidBandwidthPairs<Rest...>();
        }
    }



    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<SpectralBand, N> initializeSpectralBands()
    {
        std::array<SpectralBand, N> bands;

        // Compute the center wavelength and bandwidths for each band:
        if constexpr (spec == UNIFORM_RANGE) {
            constexpr std::array<float, 2> inputs{ Lam... };
            static_assert(inputs[0] < inputs[1]);
            static_assert(inputs[0] > 0);

            const std::array<float, N + 1> bounds = linspaceArray<float, N + 1>(inputs[0], inputs[1]);
            for (size_t i = 0; i < N; ++i) {
                if (bounds[i] < bounds[i + 1]) {
                    bands[i].minWavelength = bounds[i];
                    bands[i].maxWavelength = bounds[i + 1];
                }
                else {
                    bands[i].minWavelength = bounds[i + 1];
                    bands[i].maxWavelength = bounds[i];
                }
            }
        }
        else if constexpr (spec == BOUNDS) {
            const std::array<float, 2 * N> inputs{ Lam... };
            size_t idx = 0;
            static_assert(ValidBoundsPairs<Lam...>());
            for (size_t i = 0; i < (2 * N); i = i + 2) {
                if (inputs[i] < inputs[i + 1]) {
                    bands[idx].minWavelength = inputs[i];
                    bands[idx].maxWavelength = inputs[i + 1];
                }
                else {
                    bands[idx].minWavelength = inputs[i + 1];
                    bands[idx].maxWavelength = inputs[i];
                }
                idx++;
            }
        }
        else if constexpr (spec == BANDWIDTHS) {
            constexpr std::array<float, 2 * N> inputs{ Lam... };
            static_assert(ValidBandwidthPairs<Lam...>());
            for (size_t i = 0; i < (2 * N); i = i + 2) {
                float bandwidth = inputs[i + 1];
                bands[i].minWavelength = inputs[i] - (bandwidth / 2.f);
                bands[i].maxWavelength = inputs[i] + (bandwidth / 2.f);
            }
        }

        // Compute representative data:
        constexpr float c = SPEED_OF_LIGHT<float>();
        for (size_t i = 0; i < N; ++i) {
            // Convert wavelengths from nm to m:
            bands[i].minWavelength = bands[i].minWavelength * 1e-9f;
            bands[i].maxWavelength = bands[i].maxWavelength * 1e-9f;
            bands[i].bandwidth = bands[i].maxWavelength - bands[i].minWavelength;

            // Compute frequency bounds:
            bands[i].minFrequency = c / bands[i].maxWavelength;
            bands[i].maxFrequency = c / bands[i].minWavelength;

            // Compute average frequency, and corresponding wavelength/photon energy:
            bands[i].wavelength = (bands[i].minWavelength + bands[i].maxWavelength) / 2.f;
            bands[i].frequency = c / bands[i].wavelength;
            bands[i].photonEnergy = PhotonEnergy<float>(bands[i].wavelength);
        }

        return bands;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<float, N> initializePhotonEnergies()
    {
        std::array<float, N> photonEnergies;
        auto bands = initializeSpectralBands<N, spec, Lam...>();
        for (size_t i = 0; i < N; ++i) {
            photonEnergies[i] = bands[i].photonEnergy;
        }
        return photonEnergies;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<float, N> initializeWavelengths()
    {
        std::array<float, N> wavelengths;
        std::array<SpectralBand, N> bands = initializeSpectralBands<N, spec, Lam...>();
        for (size_t i = 0; i < N; ++i) {
            wavelengths[i] = bands[i].wavelength;
        }
        return wavelengths;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<float, N> initializeFrequencies()
    {
        std::array<float, N> frequencies;
        std::array<SpectralBand, N> bands = initializeSpectralBands<N, spec, Lam...>();
        for (size_t i = 0; i < N; ++i) {
            frequencies[i] = bands[i].frequency;
        }
        return frequencies;
    };


    template <IsFloat T>
    void readCSVPair(const fs::path& filepath, std::vector<T>& x, std::vector<T>& y)
    {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filepath << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string value1, value2;

            // Read the first column
            if (!std::getline(ss, value1, ',')) {
                std::cerr << "Error reading first column in line: " << line << std::endl;
                continue;
            }

            // Read the second column
            if (!std::getline(ss, value2, ',')) {
                std::cerr << "Error reading second column in line: " << line << std::endl;
                continue;
            }

            try {
                // Convert the string values to the desired type (float or double)
                T num1 = static_cast<T>(std::stod(value1));  // Use stod for converting to double, and cast to T
                T num2 = static_cast<T>(std::stod(value2));

                // Push the converted values into the respective vectors
                x.push_back(num1);
                y.push_back(num2);
            }
            catch (...) {
                std::cerr << "Invalid number format in line: " << line << std::endl;
            }
        }

        file.close();
    };




    // ==================== //
    // === Constructors === //
    // ==================== //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...>::SpectralData()
    {
        values_.fill(0.f);
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...>::SpectralData(std::array<float, N> values) : values_{ values }
    {

    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...>::SpectralData(const fs::path& spectralFile, bool isNanometer)
    {
        std::vector<float> newWavelengths;
        std::vector<float> valuesToSample;
        readCSVPair(spectralFile, newWavelengths, valuesToSample);

        // Convert wavelengths from nm to meters:
        if (isNanometer) {
            for (size_t i = 0; i < newWavelengths.size(); ++i) {
                newWavelengths[i] = 1e-9f * newWavelengths[i];
            }
        }

        // Perform integration:
        for (size_t i = 0; i < N; ++i) {
            values_[i] = trapezoidIntegrate(newWavelengths, valuesToSample, bands[i].minWavelength, bands[i].maxWavelength) / bands[i].bandwidth;
        }
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <IsUnit UnitType>
    SpectralData<N, spec, Lam...>::SpectralData(std::vector<UnitType> setWavelengths, const std::vector<float>& valuesToSample)
    {
        std::vector<float> newWavelengths(setWavelengths.size());
        for (size_t i = 0; i < newWavelengths.size(); ++i) {
            units::Meter val = setWavelengths[i];
            newWavelengths[i] = static_cast<float>(val.getValue());
        }

        for (size_t i = 0; i < N; ++i) {
            values_[i] = trapezoidIntegrate(newWavelengths, valuesToSample, bands[i].minWavelength, bands[i].maxWavelength) / bands[i].bandwidth;
        }
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <IsNumeric T>
    SpectralData<N, spec, Lam...>::SpectralData(T v)
    {
        values_.fill(static_cast<float>(v));
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <IsNumeric... T> requires (sizeof...(T) == N)
        SpectralData<N, spec, Lam...>::SpectralData(T... v) : values_{ static_cast<float>(v)... }
    {

    };
    

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    float SpectralData<N, spec, Lam...>::magnitude() const
    {
        float sqsum = 0;
        for (size_t i = 0; i < N; i++) {
            sqsum += values_[i] * values_[i];
        }
        return std::sqrt(sqsum);
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    float SpectralData<N, spec, Lam...>::total() const
    {
        float sum = 0;
        for (size_t i = 0; i < N; i++) {
            sum += values_[i];
        }
        return sum;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    float SpectralData<N, spec, Lam...>::mean() const
    {
        return total() / static_cast<float>(N);
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    float SpectralData<N, spec, Lam...>::integrateOver(float minLam, float maxLam) const
    {
        if (minLam > maxLam) {
            std::swap(minLam, maxLam);
        }

        if (minLam == maxLam) {
            return 0.f;
        }

        float integral = 0;
        for (size_t i = 0; i < N; ++i) {
            float bmin = bands[i].minWavelength;
            float bmax = bands[i].maxWavelength;

            // Check if there is no overlap between domains:
            if (bmin > maxLam || bmax < minLam) {
                continue;
            }

            float bound1 = std::max(bmin, minLam);
            float bound2 = std::min(bmax, maxLam);
            float dLam = bound2 - bound1;
            integral += (dLam * values_[i]);
        }

        return integral;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    float SpectralData<N, spec, Lam...>::integrate() const
    {
        float totalIntegral = 0;
        for (size_t i = 0; i < N; ++i) {
            float bmin = bands[i].minWavelength;
            float bmax = bands[i].maxWavelength;

            totalIntegral += this->integrateOver(bmin, bmax);
        }

        return totalIntegral;
    };


    // =================================== //
    // === Addition Operator Overloads === //
    // =================================== //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...>& SpectralData<N, spec, Lam...>::operator+= (const RHS& rhs)
    {
        if constexpr (std::same_as<RHS, std::array<float, N>> || std::same_as<RHS, SpectralData<N, spec, Lam...>>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] += rhs[i];
            }
        }
        else if constexpr (IsNumeric<RHS>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] += static_cast<float>(rhs);
            }
        }
        return *this;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...> SpectralData<N, spec, Lam...>::operator+ (const RHS& rhs) const
    {
        SpectralData<N, spec, Lam...> output = *this;
        output += rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator+ (T lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        SpectralData<N, spec, Lam...> output;
        if constexpr (std::same_as<T, std::array<float, N>>) {
            for (size_t i = 0; i < N; ++i) {
                output[i] = lhs[i] + rhs[i];
            }
        }
        else {
            for (size_t i = 0; i < N; ++i) {
                output[i] = static_cast<float>(lhs) + rhs[i];
            }
        }
        return output;
    };



    // ====================================== //
    // === Subtraction Operator Overloads === //
    // ====================================== //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...>& SpectralData<N, spec, Lam...>::operator-= (const RHS& rhs)
    {
        if constexpr (std::same_as<RHS, std::array<float, N>> || std::same_as<RHS, SpectralData<N, spec, Lam...>>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] -= rhs[i];
            }
        }
        else if constexpr (IsNumeric<RHS>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] -= static_cast<float>(rhs);
            }
        }
        return *this;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...> SpectralData<N, spec, Lam...>::operator- (const RHS& rhs) const
    {
        SpectralData<N, spec, Lam...> output = *this;
        output -= rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator- (T lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        SpectralData<N, spec, Lam...> output;
        if constexpr (std::same_as<T, std::array<float, N>>) {
            for (size_t i = 0; i < N; ++i) {
                output[i] = lhs[i] - rhs[i];
            }
        }
        else {
            for (size_t i = 0; i < N; ++i) {
                output[i] = static_cast<float>(lhs) - rhs[i];
            }
        }
        return output;
    };


    // ========================================= //
    // === Multiplication Operator Overloads === //
    // ========================================= //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...>& SpectralData<N, spec, Lam...>::operator*= (const RHS& rhs)
    {
        if constexpr (std::same_as<RHS, std::array<float, N>> || std::same_as<RHS, SpectralData<N, spec, Lam...>>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] *= rhs[i];
            }
        }
        else if constexpr (IsNumeric<RHS>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] *= static_cast<float>(rhs);
            }
        }
        return *this;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...> SpectralData<N, spec, Lam...>::operator* (const RHS& rhs) const
    {
        SpectralData<N, spec, Lam...> output = *this;
        output *= rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator* (T lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        SpectralData<N, spec, Lam...> output;
        if constexpr (std::same_as<T, std::array<float, N>>) {
            for (size_t i = 0; i < N; ++i) {
                output[i] = lhs[i] * rhs[i];
            }
        }
        else {
            for (size_t i = 0; i < N; ++i) {
                output[i] = static_cast<float>(lhs) * rhs[i];
            }
        }
        return output;
    };


    // =================================== //
    // === Division Operator Overloads === //
    // =================================== //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...>& SpectralData<N, spec, Lam...>::operator/= (const RHS& rhs)
    {
        if constexpr (std::same_as<RHS, std::array<float, N>> || std::same_as<RHS, SpectralData<N, spec, Lam...>>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] /= rhs[i];
            }
        }
        else if constexpr (IsNumeric<RHS>) {
            for (size_t i = 0; i < N; ++i) {
                this->values_[i] /= static_cast<float>(rhs);
            }
        }
        return *this;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    template <ValidSpectralOperand<N, spec, Lam...> RHS>
    SpectralData<N, spec, Lam...> SpectralData<N, spec, Lam...>::operator/ (const RHS& rhs) const
    {
        SpectralData<N, spec, Lam...> output = *this;
        output /= rhs;
        return output;
    };

    // Non-member (non-type operand first):
    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator/ (T lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        SpectralData<N, spec, Lam...> output;
        if constexpr (std::same_as<T, std::array<float, N>>) {
            for (size_t i = 0; i < N; ++i) {
                output[i] = lhs[i] / rhs[i];
            }
        }
        else {
            for (size_t i = 0; i < N; ++i) {
                output[i] = static_cast<float>(lhs) / rhs[i];
            }
        }
        return output;
    };


    // =================================== //
    // === Equality Operator Overlaods === //
    // =================================== //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator< (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        float mag1 = lhs.magnitude();
        float mag2 = rhs.magnitude();
        return mag1 < mag2;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator> (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        float mag1 = lhs.magnitude();
        float mag2 = rhs.magnitude();
        return mag1 > mag2;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator<= (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        float mag1 = lhs.magnitude();
        float mag2 = rhs.magnitude();
        return mag1 <= mag2;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator>= (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        float mag1 = lhs.magnitude();
        float mag2 = rhs.magnitude();
        return mag1 >= mag2;
    };

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator== (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs)
    {
        bool isEqual = true;
        for (size_t i = 0; i < N; i++) {
            isEqual = isEqual && (lhs[i] == rhs[i]);
        }
        return isEqual;
    };

    template <IsSpectral TSpectral1, IsSpectral TSpectral2>
    TSpectral2 spectralConvert_val(const TSpectral1& inputSpectrum)
    {
        TSpectral2 outputSpectrum{ 0 };
        for (size_t i = 0; i < TSpectral2::size(); ++i) {
            outputSpectrum[i] = inputSpectrum.integrateOver(TSpectral2::bands[i].minWavelength, TSpectral2::bands[i].maxWavelength) / TSpectral2::bands[i].bandwidth;
        }
        return outputSpectrum;
    };

    // Default SpectralData conversion function:
    template <IsSpectral TSpectral>
    ColorRGB spectralToRGB_val(const TSpectral& spectralData)
    {
        return spectralConvert_val<TSpectral, ColorRGB>(spectralData);
    };


    template <IsSpectral TSpectral>
    TSpectral rgbToSpectral_val(const ColorRGB& colorRGB)
    {
        return spectralConvert_val<ColorRGB, TSpectral>(colorRGB);
    };

    template <IsSpectral TSpectral>
    TSpectral blackBodyRadiance(double temperature, size_t steps)
    {
        TSpectral radiance{ 0 };
        for (size_t i = 0; i < TSpectral::size(); ++i) {
            // Compute bounds:
            double min = TSpectral::bands[i].minWavelength;
            double max = TSpectral::bands[i].maxWavelength;

            // Integrate black-body spectrum over the computed bounds:
            std::vector<double> lambda = linspace(min, max, steps);
            std::vector<double> rad = plancksLaw(temperature, lambda);

            radiance[i] = static_cast<float>(trapezoidIntegrate(lambda, rad));
        }

        return radiance;
    };

    template <IsSpectral TSpectral>
    std::ostream& operator<<(std::ostream& os, const TSpectral& spectrum)
    {
        // Save original stream state
        std::ios oldState(nullptr);
        oldState.copyfmt(os);

        // Print matrix contents
        os << "Spectral data:\n";
        for (size_t i = 0; i < spectrum.size(); ++i) {
            os << std::setprecision(2) << std::fixed << TSpectral::bands[i].minWavelength * 1e9 << "nm - ";
            os << TSpectral::bands[i].maxWavelength * 1e9 << "nm | ";
            os << std::setprecision(6) << std::scientific << spectrum[i] << "\n";
        }
        os << "\n";

        // Restore original stream state
        os.copyfmt(oldState);
        return os;
    };



    template <IsSpectral TSpectral>
    TSpectral lunarSpectralProfile()
    {
        // Determine empirically using WAC EMP Normalized Albedo Maps
        TSpectral wavelengths{ TSpectral::wavelengths };
        TSpectral spectralProfile = 1.77e6f * wavelengths - 0.145f;
        return spectralProfile;
    };
};
