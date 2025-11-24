#ifndef VIRA_DEMS_INTERFACES_SPCMAP_INTERFACE_HPP
#define VIRA_DEMS_INTERFACES_SPCMAP_INTERFACE_HPP

#include <filesystem>

#include "vira/spectral_data.hpp"
#include "vira/constraints.hpp"
#include "vira/dems/dem.hpp"
#include "vira/dems/dem_projection.hpp"

namespace fs = std::filesystem;

namespace vira::dems {
    /**
    * @brief Configuration options for SPC maplet file loading operations
    * @details Controls how Small Body Mapping Tool (SPC) maplet files are processed,
    *          including albedo handling and default surface reflectance values for
    *          planetary surface modeling applications.
    */
    struct SPCMapletOptions {
        float global_albedo = 0.06f; ///< Default albedo 
        bool load_albedos = true;    ///< Flag to read albedo data
    };

    /**
    * @brief Interface for loading SPC maplet files
    * @tparam TSpectral Spectral data type for wavelength-dependent albedo modeling
    * @tparam TFloat Floating point precision for geometric calculations
    * @tparam TMeshFloat Floating point precision for mesh data (must be >= TFloat)
    *
    * @details Provides static methods for reading specialized elevation and albedo data
    *          from planetary mapping missions.
    */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class SPCMapletInterface {
    public:
        static DEM<TSpectral, TFloat, TMeshFloat> load(const fs::path& filepath, SPCMapletOptions options = SPCMapletOptions{});

        static mat4<float> loadTransformation(const fs::path& filepath);
    };
};

#include "implementation/dems/interfaces/spcmap_interface.ipp"

#endif