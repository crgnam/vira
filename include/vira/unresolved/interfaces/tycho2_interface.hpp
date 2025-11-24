#ifndef VIRA_STARS_INTERFACES_TYCHO2_INTERFACE_HPP
#define VIRA_STARS_INTERFACES_TYCHO2_INTERFACE_HPP

#include <fstream>
#include <cstddef>
#include <string>
#include <filesystem>

#include "vira/unresolved/star.hpp"
#include "vira/unresolved/star_catalogue.hpp"

namespace fs = std::filesystem;

namespace vira::unresolved {
    class Tycho2Interface {
    public:
        inline static StarCatalogue read(const fs::path& filepath);

    private:
        inline static Star readStar(std::string& line);
        inline static size_t getNumStars(std::ifstream& file);

        static void initIrradiance(double BTmag, double VTmag, double& Vmag, double& temp, double& omega);

        inline static bool isDataLine(std::string& str);

        template <typename T>
        static void readStringBytes(std::string& str, T& val, size_t start, size_t stop);
    };
};

#include "implementation/unresolved/interfaces/tycho2_interface.ipp"

#endif