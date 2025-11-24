#ifndef VIRA_UTILS_COLOR_MAP_HPP 
#define VIRA_UTILS_COLOR_MAP_HPP

#include <vector>

namespace vira::images {
    ColorRGB colorMap(float value, std::vector<ColorRGB>& colormap, std::vector<float>& key);
}

namespace vira::colormaps {
    std::vector<ColorRGB> blank();

    std::vector<ColorRGB> viridis();
    std::vector<ColorRGB> turbo();
    std::vector<ColorRGB> jet();
    std::vector<ColorRGB> hsv();
    std::vector<ColorRGB> rainbow();
    std::vector<ColorRGB> hot();
    std::vector<ColorRGB> cool();
    std::vector<ColorRGB> bone();
};

#include "implementation/images/color_map.ipp"

#endif