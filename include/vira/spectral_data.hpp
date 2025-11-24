#ifndef VIRA_SPECTRAL_DATA_HPP
#define VIRA_SPECTRAL_DATA_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <filesystem>
#include <ostream>
#include <iomanip>

#include "vira/math.hpp"
#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/units/units.hpp"

namespace fs = std::filesystem;

namespace vira {
    enum SpectralSpec {
        UNIFORM_RANGE,
        BOUNDS,
        BANDWIDTHS
    };

    struct SpectralBand {
        float frequency; // Average
        float photonEnergy; // Average
        float wavelength; // Computed from average frequency

        float bandwidth;
        float minWavelength;
        float maxWavelength;
        float minFrequency;
        float maxFrequency;
    };

    template <size_t N, SpectralSpec spec, int... Lam>
    concept ValidLam = (((sizeof...(Lam) == (2 * N)) && (spec != UNIFORM_RANGE)) || ((sizeof...(Lam) == 2) && (spec == UNIFORM_RANGE)));

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<SpectralBand, N> initializeSpectralBands();

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<float, N> initializePhotonEnergies();

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<float, N> initializeWavelengths();

    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    constexpr std::array<float, N> initializeFrequencies();

    template <IsFloat T>
    void readCSVPair(const fs::path& filepath, std::vector<T>& x, std::vector<T>& y);


    // Forward Declare:
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    class SpectralData;

    // TODO Write better constraints!
    template <typename T>
    concept HasSize = requires(T t) {
        { T::size() } -> std::same_as<size_t>;
    };

    template <typename T>
    concept IsSpectral = requires(T x) {
        { SpectralData{ x } } -> std::same_as<T>;
    };

    template <typename T, size_t N, SpectralSpec spec, int... Lam>
    concept NonSpectralOperand =
        IsNumeric<T> ||
        std::same_as<T, std::array<float, N>>;

    template<typename T, size_t N, SpectralSpec spec, int... Lam>
    concept ValidSpectralOperand =
        NonSpectralOperand<T, N, spec, Lam...> ||
        std::same_as<T, SpectralData<N, spec, Lam...>>;


    // =========================== //
    // === Spectral Data Class === //
    // =========================== //
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    class SpectralData {
    public:
        SpectralData();
        SpectralData(std::array<float, N> values);
        SpectralData(const fs::path& spectralFile, bool isNanometer = true);

        template <IsUnit UnitType>
        SpectralData(std::vector<UnitType> setWavelengths, const std::vector<float>& value);

        template <IsNumeric T>
        SpectralData(T v);

        template <IsNumeric... T> requires (sizeof...(T) == N)
            SpectralData(T... v);


        const float& operator[] (size_t i) const { return values_[i]; }
        float& operator[] (size_t idx) { return const_cast<float&>(const_cast<const SpectralData*>(this)->operator[](idx)); }

        float magnitude() const;
        float total() const;
        float mean() const;

        float integrateOver(float minLam, float maxLam) const;
        float integrate() const;

        // Iterator support for range-based for loops
        float* begin() { return values_.data(); }
        float* end() { return values_.data() + N; }
        const float* begin() const { return values_.data(); }
        const float* end() const { return values_.data() + N; }

        // Addition operator overloads:
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData& operator+= (const RHS& rhs);
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData operator+ (const RHS& rhs) const;

        // Subtraction operator overloads:
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData& operator-= (const RHS& rhs);
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData operator- (const RHS& rhs) const;

        // Multiplication operator overloads:
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData& operator*= (const RHS& rhs);
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData operator* (const RHS& rhs) const;

        // Division operator overloads:
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData& operator/= (const RHS& rhs);
        template <ValidSpectralOperand<N, spec, Lam...> RHS>
        SpectralData operator/ (const RHS& rhs) const;

        SpectralData operator-() const {
            SpectralData result;
            for (size_t i = 0; i < N; ++i) {
                result.values_[i] = -values_[i];
            }
            return result;
        }


        // Details about the spectrum:
        std::array<float, N> values_;
        static constexpr size_t size() { return N; }
        constexpr static const std::array<SpectralBand, N> bands = initializeSpectralBands<N, spec, Lam...>();
        constexpr static const std::array<float, N> photonEnergies = initializePhotonEnergies<N, spec, Lam...>();
        constexpr static const std::array<float, N> wavelengths = initializeWavelengths<N, spec, Lam...>();
        constexpr static const std::array<float, N> frequencies = initializeFrequencies<N, spec, Lam...>();
    };

    // Right-hand-side operator overloads:
    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator+ (T lhs, const SpectralData<N, spec, Lam...>& rhs);

    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator- (T lhs, const SpectralData<N, spec, Lam...>& rhs);

    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator* (T lhs, const SpectralData<N, spec, Lam...>& rhs);

    template <size_t N, SpectralSpec spec, int... Lam, NonSpectralOperand<N, spec, Lam...> T> requires ValidLam<N, spec, Lam...>
    SpectralData<N, spec, Lam...> operator/ (T lhs, const SpectralData<N, spec, Lam...>& rhs);


    // Equality operator overloads:
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator<  (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs);
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator>  (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs);
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator<= (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs);
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator>= (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs);
    template <size_t N, SpectralSpec spec, int... Lam> requires ValidLam<N, spec, Lam...>
    bool operator== (const SpectralData<N, spec, Lam...>& lhs, const SpectralData<N, spec, Lam...>& rhs);


    // Typedef useful spectral types:
    template <size_t N, auto Lam1, auto Lam2>
    using UniformSpectralData = SpectralData<N, UNIFORM_RANGE, Lam1, Lam2>;

    template <size_t N, int... Lam>
    using SpectralBandData = SpectralData<N, BOUNDS, Lam...>;

    template <size_t N, int... Lam>
    using SpectralBandwidthsData = SpectralData<N, BANDWIDTHS, Lam...>;

    template <size_t N>
    using UniformVisibleData = SpectralData<N, UNIFORM_RANGE, 380, 750>;

    typedef UniformVisibleData<1> Visible_1bin;
    typedef UniformVisibleData<3> Visible_3bin;
    typedef UniformVisibleData<8> Visible_8bin;

    // Define a representative RGB color-space:
    typedef SpectralData<3, BOUNDS, 600, 750, 500, 600, 380, 500> ColorRGB;

    // Default SpectralData conversion to/from ColorRGB functions:
    template <IsSpectral TSpectral1, IsSpectral TSpectral2>
    TSpectral2 spectralConvert_val(const TSpectral1& inputSpectrum);

    template <IsSpectral TSpectral>
    ColorRGB spectralToRGB_val(const TSpectral& spectralData);

    template <IsSpectral TSpectral>
    TSpectral rgbToSpectral_val(const ColorRGB& ColorRGB);

    template <IsSpectral TSpectral>
    TSpectral blackBodyRadiance(double temperature, size_t steps);

    template <IsSpectral TSpectral>
    std::ostream& operator<<(std::ostream& os, const TSpectral& spectrum);


    template <IsSpectral TSpectral>
    TSpectral lunarSpectralProfile();
};

template <vira::IsSpectral TSpectral>
struct std::hash<TSpectral> {
    size_t operator()(const TSpectral& SpectralData) const noexcept {
        size_t hash = 0;
        std::hash<float> hasher;
        for (size_t i = 0; i < TSpectral::size(); ++i) {
            hash ^= hasher(SpectralData[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

#include "implementation/spectral_data.ipp"

#endif
