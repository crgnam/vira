#ifndef VIRA_UTILS_GEO_TRANSLATE_HPP
#define VIRA_UTILS_GEO_TRANSLATE_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace vira::tools {
    inline void geoTranslate(const fs::path& inputFile, const fs::path& outputFile);

    inline void geoTranslateInMemory(vira::dems::GeoreferenceImage<float>& geoImage);

    inline vira::dems::GeoreferenceImage<float> geoTranslateFromFile(const fs::path& inputFile, vira::dems::GDALOptions options = vira::dems::GDALOptions{});
};

#include "implementation/tools/geo_translate.ipp"

#endif