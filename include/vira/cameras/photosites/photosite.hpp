#ifndef VIRA_CAMERAS_PHOTOSITES_PHOTOSITE_HPP
#define VIRA_CAMERAS_PHOTOSITES_PHOTOSITE_HPP

#include <cstddef>

#include "vira/constraints.hpp"
#include "vira/spectral_data.hpp"

namespace vira::cameras {
    /**
     * @brief Configuration structure for camera photosite parameters
     * @tparam TSpectral Spectral data type for quantum efficiency modeling
     *
     * Contains the parameters needed to model the response characteristics of
     * individual photosites in a camera photosite, including quantum efficiency,
     * gain settings, and digital conversion parameters.
     */
    template <IsSpectral TSpectral>
    struct PhotositeConfig {
        PhotositeConfig();

        size_t bit_depth = 8;                   ///< ADC bit depth for digital conversion
        size_t well_depth = 15000;              ///< Electron well capacity (full well depth)
                                                
        float gain = 0.333333f;                 ///< Photosite gain in ADU/electron
        float unity_gain_db = 0.f;              ///< Unity gain reference in dB

        TSpectral quantum_efficiency;           ///< Spectral quantum efficiency curve
        vira::ColorRGB quantum_efficiency_rgb;  ///< RGB quantum efficiency for fast computation

        float linear_scale_factor = 1.f;        ///< Linear scaling factor for output scaling (tuning parameter)

        template <IsUnit UnitType>
        void setQuantumEfficiency(std::vector<UnitType> wavelengths, const std::vector<float>& qe); ///< Set Quantum Efficiency

        void setGainDB(double gain_db, double new_unity_gain_db = 0); ///< Set gain in units of dB
        float getGainDB(); ///< Get the current gain in units of dB

    private:
        TSpectral defaultQE();
        vira::ColorRGB defaultQE_RGB();
    };

    /**
     * @brief Camera photosite response model
     * @tparam TSpectral Spectral data type for quantum efficiency modeling
     *
     * Models the response of individual camera photosites, converting
     * incident photons to digital output values through quantum efficiency,
     * noise, gain, and analog-to-digital conversion simulation.
     */
    template <IsSpectral TSpectral>
    class Photosite {
    public:
        Photosite() = default;
        Photosite(PhotositeConfig<TSpectral> config);

        virtual ~Photosite() = default;

        // Delete copy operations
        Photosite(const Photosite&) = delete;
        Photosite& operator=(const Photosite&) = delete;

        // Allow move operations
        Photosite(Photosite&&) = default;
        Photosite& operator=(Photosite&&) = default;

        virtual void setGain(double new_gain) { config_.gain = static_cast<float>(new_gain); }
        virtual void setGainDB(double new_gain_db, double unity_gain_db = 0) { config_.setGainDB(new_gain_db, unity_gain_db); }

        virtual float exposePixel(TSpectral photon_count, float noise_count);
        virtual vira::ColorRGB exposePixelRGB(vira::ColorRGB photon_count, vira::ColorRGB noise_count);

    private:
        PhotositeConfig<TSpectral> config_{};
        float max_adu_;
    };
}

#include "implementation/cameras/photosites/photosite.ipp"

#endif