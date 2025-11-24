#include <cstddef>

#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"

namespace vira::cameras {
    /**
     * @brief Creates a Bayer color filter array mosaic
     * @tparam TSpectral Spectral data type for filter transmission curves
     * @param resolution Image resolution for the filter mosaic
     * @param red_filter Spectral transmission curve for red filter elements
     * @param green_filter Spectral transmission curve for green filter elements
     * @param blue_filter Spectral transmission curve for blue filter elements
     * @return Image containing the Bayer filter mosaic pattern
     * @details Generates the standard RGGB Bayer pattern:
     *          - Row 0: R G R G ...
     *          - Row 1: G B G B ...
     *          - Row 2: R G R G ... (pattern repeats)
     *          Each pixel contains the spectral transmission of its corresponding color filter.
     */
    template <IsSpectral TSpectral>
    vira::images::Image<TSpectral> bayerFilter(vira::images::Resolution resolution, TSpectral red_filter, TSpectral green_filter, TSpectral blue_filter)
    {
        vira::images::Image<TSpectral> filter_mosaic(resolution);

        for (int i = 0; i < resolution.x; ++i) {
            for (int j = 0; j < resolution.y; ++j) {
                if ((i % 2) == 0) {
                    // Even columns: R/G pattern
                    if ((j % 2) == 0) {
                        filter_mosaic(i, j) = red_filter;
                    }
                    else {
                        filter_mosaic(i, j) = green_filter;
                    }
                }
                else {
                    // Odd columns: G/B pattern  
                    if ((j % 2) == 0) {
                        filter_mosaic(i, j) = green_filter;
                    }
                    else {
                        filter_mosaic(i, j) = blue_filter;
                    }
                }
            }
        }

        return filter_mosaic;
    }
}