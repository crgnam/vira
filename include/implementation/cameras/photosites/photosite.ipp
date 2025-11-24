#include <fstream>
#include <memory>
#include <random>

#include "vira/math.hpp"
#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"
#include "vira/utils/utils.hpp"
#include "vira/constraints.hpp"

namespace vira::cameras {
    // ======================== //
    // === Photosite Config === //
    // ======================== //

    /**
     * @brief Default constructor initializing Photosite configuration with default quantum efficiency
     */
    template <IsSpectral TSpectral>
    PhotositeConfig<TSpectral>::PhotositeConfig()
    {
        quantum_efficiency = defaultQE();
        quantum_efficiency_rgb = defaultQE_RGB();
    }

    /**
     * @brief Sets Photosite gain using decibel notation
     * @param gain_db Gain value in decibels
     * @param new_unity_gain_db New unity gain reference in dB
     * @details Converts dB gain to linear gain using: \f$\text{gain} = 10^{(\text{gain\_dB} - \text{unity\_gain\_dB})/20}\f$
     */
    template <IsSpectral TSpectral>
    void PhotositeConfig<TSpectral>::setGainDB(double gain_db, double new_unity_gain_db)
    {
        unity_gain_db = static_cast<float>(new_unity_gain_db);
        gain = std::pow(10.f, ((static_cast<float>(gain_db) - unity_gain_db) / 20.f));
    }

    /**
     * @brief Gets current Photosite gain in decibels
     * @return Current gain in dB relative to unity gain reference
     * @details Converts linear gain to dB using: \f$\text{gain\_dB} = 20 \log_{10}(\text{gain}) + \text{unity\_gain\_dB}\f$
     */
    template <IsSpectral TSpectral>
    float PhotositeConfig<TSpectral>::getGainDB()
    {
        return (20.f * std::log10(gain) + unity_gain_db);
    }

    /**
     * @brief Sets quantum efficiency from wavelength-dependent data
     * @tparam UnitType Wavelength unit type (e.g., nanometers, micrometers)
     * @param wavelengths Vector of wavelength values
     * @param qe Vector of quantum efficiency values (0-1 range)
     */
    template <IsSpectral TSpectral>
    template <IsUnit UnitType>
    void PhotositeConfig<TSpectral>::setQuantumEfficiency(std::vector<UnitType> wavelengths, const std::vector<float>& qe)
    {
        quantum_efficiency = TSpectral(wavelengths, qe);
    }

    /**
     * @brief Generates default spectral quantum efficiency curve
     * @return Default quantum efficiency weighted by photon energy
     * @details Creates a realistic QE curve that scales with photon energy,
     *          simulating typical silicon photodiode response characteristics
     */
    template <IsSpectral TSpectral>
    TSpectral PhotositeConfig<TSpectral>::defaultQE()
    {
        float total_energy = 0;
        for (size_t i = 0; i < TSpectral::size(); ++i) {
            total_energy += TSpectral::bands[i].photonEnergy / static_cast<float>(TSpectral::size());
        }

        float scale = 0.5f;
        TSpectral default_qe{ 0 };
        for (size_t i = 0; i < default_qe.size(); ++i) {
            default_qe[i] = scale * TSpectral::bands[i].photonEnergy / total_energy;
        }

        return default_qe;
    }

    /**
     * @brief Generates default RGB quantum efficiency values
     * @return RGB quantum efficiency converted from spectral default
     */
    template <IsSpectral TSpectral>
    vira::ColorRGB PhotositeConfig<TSpectral>::defaultQE_RGB()
    {
        return spectralToRGB_val<TSpectral>(defaultQE());
    }



    // ============== //
    // === Photosite === //
    // ============== //

    /**
     * @brief Constructs a photosite with specified configuration
     * @param config Photosite configuration parameters
     * @details Initializes maximum ADU value based on bit depth and linear scale factor
     */
    template <IsSpectral TSpectral>
    Photosite<TSpectral>::Photosite(PhotositeConfig<TSpectral> config) :
        config_(config)
    {
        max_adu_ = config_.linear_scale_factor * (std::pow(2.f, static_cast<float>(config_.bit_depth)) - 1.f);
    }

    /**
     * @brief Simulates photosite exposure to spectral photon flux
     * @param photon_count Incident photon count per wavelength band
     * @param noise_count Additional noise electrons (dark current, readout noise)
     * @return Normalized digital output value (0-1 range)
     * @details Converts photons to electrons using quantum efficiency, adds noise,
     *          applies gain, and normalizes to maximum ADU range
     * @todo Implement non-linear response near well saturation
     * @todo Account for actual well depth limitations
     */
    template <IsSpectral TSpectral>
    float Photosite<TSpectral>::exposePixel(TSpectral photon_count, float noise_count)
    {
        // Convert photons to electrons using quantum efficiency
        TSpectral electron_count_per_wavelength = config_.quantum_efficiency * photon_count;
        float total_electron_count = electron_count_per_wavelength.total() + noise_count;

        // Apply gain and clamp to maximum ADU
        float amplified_electrons = std::min(config_.gain * total_electron_count, max_adu_);

        return amplified_electrons / max_adu_;
    }

    /**
     * @brief Simulates photosite exposure to RGB photon flux
     * @param photon_count Incident photon count per RGB channel
     * @param noise_count Noise electrons per RGB channel
     * @return Normalized RGB digital output values (0-1 range per channel)
     * @details Fast RGB processing path using pre-computed RGB quantum efficiency
     */
    template <IsSpectral TSpectral>
    ColorRGB Photosite<TSpectral>::exposePixelRGB(ColorRGB photon_count, ColorRGB noise_count)
    {
        // Convert photons to electrons and add noise per channel
        ColorRGB electron_count_per_color = (config_.quantum_efficiency_rgb * photon_count) + noise_count;

        // Apply gain
        ColorRGB amplified_electrons = config_.gain * electron_count_per_color;

        return amplified_electrons / max_adu_;
    }
}