#ifndef VIRA_CAMERAS_SENSORS_FILTER_ARRAYS_HPP
#define VIRA_CAMERAS_SENSORS_FILTER_ARRAYS_HPP

#include "vira/spectral_data.hpp"
#include "vira/images/image.hpp"

namespace vira::cameras {
    template <IsSpectral TSpectral>
    vira::images::Image<TSpectral> bayerFilter(vira::images::Resolution resolution, TSpectral red_filter, TSpectral green_filter, TSpectral blue_filter);
}

#include "implementation/cameras/filter_arrays.ipp"

#endif