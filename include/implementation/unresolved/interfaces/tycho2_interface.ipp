#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cstddef>

#include "vira/math.hpp"
#include "vira/unresolved/magnitudes.hpp"
#include "vira/unresolved/star_catalogue.hpp"
#include "vira/utils/valid_value.hpp"
#include "vira/utils/utils.hpp"

namespace fs = std::filesystem;

namespace vira::unresolved {
    StarCatalogue Tycho2Interface::read(const fs::path& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Error opening file: " + filepath.string());
        }

        size_t numStars = getNumStars(file);

        std::vector<Star> stars(numStars);
        
        size_t i = 0;
        std::string line;
        while (std::getline(file, line)) {
            if (isDataLine(line)) {
                Star newStar = readStar(line);
                if (newStar.isInitialized()) {
                    stars[i] = newStar;
                    i++;
                }
            }
        }
        file.close();

        // Remove unecessary allocations (due to bad stars):
        stars.erase(stars.begin() + static_cast<std::ptrdiff_t>(i), stars.end());

        return StarCatalogue(stars);
    };

    Star Tycho2Interface::readStar(std::string& line)
    {
        double BTmag;
        double VTmag;
        double RAdeg;
        double DEdeg;
        double pmRA;
        double pmDE;

        // Read Tycho BT and VT magnitudes:            
        readStringBytes(line, BTmag, 111, 116);
        readStringBytes(line, VTmag, 124, 129);

        // Read proper motion if available:
        readStringBytes(line, RAdeg, 16, 27);
        readStringBytes(line, DEdeg, 29, 40);
        readStringBytes(line, pmRA, 42, 48);
        readStringBytes(line, pmDE, 50, 56);

        if (vira::utils::IS_INVALID(RAdeg)) {
            readStringBytes(line, RAdeg, 153, 164);
            pmRA = 0;
        }

        if (vira::utils::IS_INVALID(DEdeg)) {
            readStringBytes(line, DEdeg, 166, 177);
            pmDE = 0;
        }

        // Create the star:
        double Vmag = vira::utils::INVALID_VALUE<double>();
        double temp = vira::utils::INVALID_VALUE<double>();
        double omega = vira::utils::INVALID_VALUE<double>();
        initIrradiance(BTmag, VTmag, Vmag, temp, omega);

        if (vira::utils::IS_INVALID(Vmag)) {
            return Star();
        }

        // Convert from degrees to radians:
        double RA = DEG2RAD<double>() * RAdeg; // Radians
        double DE = DEG2RAD<double>() * DEdeg; // Radians

        // Convert from milliarcseconds to radians:
        double masToDeg = 1. / (3600. * 1000.);
        pmRA = DEG2RAD<double>() * masToDeg * pmRA; // Radians per year
        pmDE = DEG2RAD<double>() * masToDeg * pmDE; // Radians per year

        return Star(RA, DE, pmRA, pmDE, Vmag, temp, omega);
    };

    size_t Tycho2Interface::getNumStars(std::ifstream& file)
    {
        std::string line;
        size_t numStars = 0;
        while (std::getline(file, line)) {
            if (isDataLine(line)) {
                ++numStars;
            }
        }
        file.clear();
        file.seekg(0);

        return numStars;
    };

    void Tycho2Interface::initIrradiance(double BTmag, double VTmag, double& Vmag, double& temp, double& omega)
    {
        // Conversions here are approximate.  Consider using more complex transformations
        // As shown here: https://www.cosmos.esa.int/documents/532822/552851/vol1_all.pdf
        // ("The Hipparcos and Tycho Catalogues" Astrometric and Photometric Star Catalogues derived from the ESA Hipparcos Space Astrometry Mission)
        // Temperature approximation: https://arxiv.org/abs/1201.1809

        double BVcolorIndex = 0.3; // Default to a white colored star
        if (vira::utils::IS_INVALID(BTmag) && vira::utils::IS_VALID(VTmag)) {
            Vmag = VTmag;
        }
        else if (vira::utils::IS_VALID(BTmag) && vira::utils::IS_INVALID(VTmag)) {
            Vmag = BTmag;
        }
        else if (vira::utils::IS_INVALID(BTmag) && vira::utils::IS_INVALID(VTmag)) {
            return;
        }
        else {
            Vmag = VTmag - 0.090 * (BTmag - VTmag);
            BVcolorIndex = 0.850 * (BTmag - VTmag);
        }
        temp = 4600. * (1. / (0.92 * BVcolorIndex + 1.7) + 1. / (0.92 * BVcolorIndex + 0.62));


        // Compute photon flux from reference:
        double flux = vira::unresolved::Vband.fluxFromMagnitude(Vmag);

        // Compute solid angle:
        size_t N = 1000;
        std::vector<double> lambda;
        std::vector<double> vBandEfficiency = vira::unresolved::johnsonVBandApproximation(N, lambda);
        std::vector<double> radiance = plancksLaw(temp, lambda);
        std::vector<double> photonCounts(N);
        for (size_t i = 0; i < N; ++i) {
            photonCounts[i] = vBandEfficiency[i] * radiance[i] / PhotonEnergy<double>(lambda[i]);
        }
        double flux2 = trapezoidIntegrate(lambda, photonCounts);
        omega = flux / flux2;
    };

    static bool stringIsFloat(const std::string& s) {
        std::istringstream iss(s);
        float f;
        // Try parsing as float and make sure the entire string is consumed
        return (iss >> f) && (iss.eof());
    }

    bool Tycho2Interface::isDataLine(std::string& str)
    {
        if (str.length() < 185) {
            return false;
        }

        // Extract substrings from character positions 16-27 and 29-40
        std::string field1 = str.substr(16, 11);  // 0-based index, length 12
        std::string field2 = str.substr(29, 11);

        // Trim leading/trailing spaces
        field1.erase(0, field1.find_first_not_of(' '));
        field1.erase(field1.find_last_not_of(' ') + 1);
        field2.erase(0, field2.find_first_not_of(' '));
        field2.erase(field2.find_last_not_of(' ') + 1);

        return stringIsFloat(field1) && stringIsFloat(field2);
    };

    template <typename T>
    void Tycho2Interface::readStringBytes(std::string& str, T& val, size_t start, size_t stop)
    {
        // Get the substring:
        start = start - 1;
        std::string sub = str.substr(start, stop - start);

        if (utils::isWhitespace(sub)) {
            val = vira::utils::INVALID_VALUE<T>();
            return;
        }

        if constexpr (std::same_as<T, double>) {
            val = std::stod(sub);
        }
        else if constexpr (std::same_as<T, float>) {
            val = std::stof(sub);
        }
        else if constexpr (std::same_as<T, std::string>) {
            val = sub;
        }
        else if constexpr (std::is_integral_v<T>) {
            val = static_cast<T>(std::stoi(sub));
        }
    }
};