#ifndef VIRA_UTILS_HPP
#define VIRA_UTILS_HPP

#include <string>
#include <vector>
#include <filesystem>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/images/resolution.hpp"

namespace fs = std::filesystem;

namespace vira::utils {
    // ====================== //
    // === Path Utilities === //
    // ====================== //
    inline std::filesystem::path getExecutablePath();

    inline std::string getFileName(const std::filesystem::path path);

    inline std::vector<std::string> getFiles(fs::path directory);

    inline void makePath(const std::string& pathStr);

    inline void makePath(std::filesystem::path path);

    template <size_t N>
    std::string padZeros(auto number);

    inline bool isWhitespace(const std::string& str);

    inline bool isGlob(const std::string& str);

    inline void validateDirectory(const fs::path& dirpath);

    inline void validateFile(const fs::path& filepath);

    inline fs::path combinePaths(const fs::path& root, const fs::path& appendingPath, bool validate = true);

    inline std::vector<std::string> expandGlobList(std::vector<std::string> inputList);

    inline void validateFiles(std::vector<std::string> files, std::string typeDesc);

    inline bool sameString(const std::string& str1, const std::string& str2);


    // ==================================== //
    // === Byte (Endian-ness) Utilities === //
    // ==================================== //
    inline void reverseFloat(float& value);

    inline void reverseInt16(int16_t& value);

    inline void readBuffer(void* buffer, size_t size, size_t count, FILE* file);

    

    // ================================== //
    // === Array and Vector Utilities === //
    // ================================== //
    template <typename T>
    void getUnique(std::vector<T>& input);

    // Array caster:
    template <IsFloat T1, IsFloat T2, size_t N>
    std::array<T1, N> castArray(std::array<T2, N> input);

    // Convert linear to subscript vertices:
    inline void linear2sub(size_t idx, vira::images::Resolution resolution, size_t& i, size_t& j);


    // ====================== //
    // === Geometry Utils === //
    // ====================== //
    template <IsFloat TFloat>
    vec3<TFloat> ellipsoidToCartesian(double lon, double lat, double alt, double a, double b, double c = -1.);
};

#include "implementation/utils/utils.ipp"

#endif