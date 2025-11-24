#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <regex>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <climits>
#elif __linux__ 
#include <unistd.h>
#endif

#include "vira/vec.hpp"
#include "vira/math.hpp"
#include "vira/constraints.hpp"
#include "vira/images/image.hpp"
#include "vira/geometry/vertex.hpp"

namespace fs = std::filesystem;

namespace vira::utils {
    // ====================== //
    // === Path Utilities === //
    // ====================== //
    std::filesystem::path getExecutablePath()
    {
#ifdef _WIN32
        // Windows specific
        wchar_t szPath[MAX_PATH];
        GetModuleFileNameW(nullptr, szPath, MAX_PATH);
        return std::filesystem::path{ szPath }.parent_path() / "";
#elif __APPLE__
        char szPath[PATH_MAX];
        std::uint32_t bufsize = PATH_MAX;
        if (!_NSGetExecutablePath(szPath, &bufsize))
            return std::filesystem::path{ szPath }.parent_path() / ""; // to finish the folder path with (back)slash
        return {};  // some error
#else
        // Linux specific
        char szPath[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", szPath, PATH_MAX);
        if (count < 0 || count >= PATH_MAX)
            return {}; // some error
        szPath[count] = '\0';
        return std::filesystem::path{ szPath }.parent_path() / "";
#endif
    };

    std::string getFileName(const fs::path path)
    {
        std::string fileName = path.filename().string();
        size_t position = fileName.find(".");
        fileName = (std::string::npos == position) ? fileName : fileName.substr(0, position);
        return fileName;
    };

    static bool matchesGlob(const std::string& filename, const std::string& pattern) {
        std::string regexPattern = pattern;

        // Escape special regex characters except * and ?
        std::regex specialChars(R"([.^$+{}[\]|()\\])");
        regexPattern = std::regex_replace(regexPattern, specialChars, R"(\$&)");

        // Convert glob wildcards to regex
        regexPattern = std::regex_replace(regexPattern, std::regex(R"(\*)"), ".*");
        regexPattern = std::regex_replace(regexPattern, std::regex(R"(\?)"), ".");

        return std::regex_match(filename, std::regex(regexPattern));
    }

    std::vector<std::string> getFiles(fs::path filepath)  // Keep original signature
    {
        if (filepath.is_relative()) {
            filepath = fs::absolute(filepath);  // Make entire path absolute first
        }

        fs::path directory = filepath.parent_path();
        std::string filename_pattern = filepath.filename().string();

        if (directory.empty()) directory = ".";

        std::vector<std::string> files;

        if (fs::is_directory(directory)) {
            for (const auto& entry : fs::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (matchesGlob(filename, filename_pattern)) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        }

        return files;
    }

    void makePath(const std::string& pathStr)
    {
        const fs::path path = pathStr;
        makePath(path);
    };

    void makePath(fs::path path)
    {
        if (!path.empty()) {
            path.remove_filename();
            if (!path.empty()) {
                std::filesystem::create_directories(path);
            }
        }
    };

    template <size_t N>
    std::string padZeros(auto number) {
        std::string numStr = std::to_string(number);
        numStr = std::string(N - std::min(N, numStr.length()), '0') + numStr;
        return numStr;
    };

    bool isWhitespace(const std::string& str) {
        for (char c : str) {
            if (!std::isspace(c)) {
                return false;
            }
        }
        return true;
    };

    bool isGlob(const std::string& str)
    {
        const std::string glob_chars = "*?[]{}";
        return std::any_of(str.begin(), str.end(), [&glob_chars](char c) {
            return glob_chars.find(c) != std::string::npos;
            });
    }

    void validateDirectory(const fs::path& dirpath)
    {
        // Check if the path exists and is a directory
        if (fs::exists(dirpath) && fs::is_directory(dirpath)) {
            return;
        }
        throw std::runtime_error(dirpath.string() + " is not a valid directory");
    };

    void validateFile(const fs::path& filepath)
    {
        // Check if the path exists and is a regular file
        if (fs::exists(filepath) && fs::is_regular_file(filepath)) {
            return;
        }
        throw std::runtime_error(filepath.string() + " is not a valid file path");
    };

    fs::path combinePaths(const fs::path& root, const fs::path& appendingPath, bool validate)
    {
        if (!root.empty()) {
            fs::path outputPath;
            if (appendingPath.is_absolute()) {
                outputPath = appendingPath;
            }
            else {
                outputPath = root / appendingPath;
            }

            if (validate) {
                validateFile(outputPath);
            }

            return outputPath;
        }
        else {
            return appendingPath;
        }
    };

    std::vector<std::string> expandGlobList(std::vector<std::string> inputList)
    {
        std::vector<std::string> expandedInputList;
        for (auto& input : inputList) {
            if (isGlob(input)) {
                auto inputExpanded = vira::utils::getFiles(input);
                for (auto& ie : inputExpanded) {
                    expandedInputList.push_back(ie);
                }
            }
            else {
                expandedInputList.push_back(input);
            }
        }
        return expandedInputList;
    };

    void validateFiles(std::vector<std::string> files, std::string typeDesc)
    {
        for (auto& file : files) {
            try {
                validateFile(file);
            }
            catch (std::exception& e) {
                throw std::runtime_error(typeDesc + " | " + std::string(e.what()));
            }
        }
    };

    bool sameString(const std::string& str1, const std::string& str2) {
        return str1.length() == str2.length() &&
            std::equal(str1.begin(), str1.end(), str2.begin(),
                [](char a, char b) {
                    return std::tolower(a, std::locale()) == std::tolower(b, std::locale());
                });
    }



    // ==================================== //
    // === Byte (Endian-ness) Utilities === //
    // ==================================== //
    void reverseFloat(float& value) {
        // Create a byte array from the float
        std::array<std::byte, 4> bytes = std::bit_cast<std::array<std::byte, 4>>(value);

        // Swap bytes
        std::swap(bytes[0], bytes[3]);
        std::swap(bytes[1], bytes[2]);

        // Convert back to float
        value = std::bit_cast<float>(bytes);
    };

    void reverseInt16(int16_t& value) {
        // Create a byte array from the int16_t
        std::array<std::byte, 2> bytes = std::bit_cast<std::array<std::byte, 2>>(value);

        // Swap bytes
        std::swap(bytes[0], bytes[1]);

        // Convert back to int16_t
        value = std::bit_cast<int16_t>(bytes);
    }

    void readBuffer(void* buffer, size_t size, size_t count, FILE* file)
    {
        size_t freadReturn = fread(buffer, size, count, file);
        if (freadReturn != count) {
            throw std::runtime_error("Failed reading file");
        }
    };

    

    // ================================== //
    // === Array and Vector Utilities === //
    // ================================== //
    template <typename T>
    void getUnique(std::vector<T>& input)
    {
        std::sort(input.begin(), input.end());
        std::vector<size_t>::iterator it;
        it = unique(input.begin(), input.end());
        input.resize(std::distance(input.begin(), it));
    };

    template <IsFloat T1, IsFloat T2, size_t N>
    std::array<T1, N> castArray(std::array<T2, N> input) {
        std::array<T1, N> output;
        for (size_t i = 0; i < N; i++) {
            output[i] = static_cast<T1>(input[i]);
        }
        return output;
    };

    void linear2sub(size_t idx, vira::images::Resolution resolution, size_t& i, size_t& j)
    {
        i = idx % static_cast<size_t>(resolution.x);
        j = (idx - i) / static_cast<size_t>(resolution.x);
    };


    // ====================== //
    // === Geometry Utils === //
    // ====================== //
    template <IsFloat TFloat>
    vec3<TFloat> ellipsoidToCartesian(double lon, double lat, double alt, double a, double b, double c)
    {
        if (c < 0) {
            c = a;
        }

        double ex2 = (a * a - c * c) / (a * a);
        double ee2 = (a * a - b * b) / (a * a);
        constexpr double deg2rad = PI<double>() / 180.;

        double clat = std::cos(lat * deg2rad);
        double slat = std::sin(lat * deg2rad);
        double clon = std::cos(lon * deg2rad);
        double slon = std::sin(lon * deg2rad);

        double V = a / std::sqrt(1 - ex2 * (slat * slat) - ee2 * (clat * clat) * (slon * slon));

        vec3<TFloat> vert;
        vert[0] = static_cast<TFloat>((V + alt) * clon * clat);
        vert[1] = static_cast<TFloat>((V * (1 - ee2) + alt) * slon * clat);
        vert[2] = static_cast<TFloat>((V * (1 - ex2) + alt) * slat);

        return vert;
    };
};