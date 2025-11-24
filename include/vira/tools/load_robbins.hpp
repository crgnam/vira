#ifndef VIRA_TOOLS_LOAD_ROBBINS_HPP
#define VIRA_TOOLS_LOAD_ROBBINS_HPP

#include <vector>
#include <string>
#include <cstddef>
#include <limits>
#include <filesystem>
#include <sstream>
#include <stdexcept>

#include "vira/constraints.hpp"
#include "vira/vec.hpp"
#include "vira/geometry/ellipsoid.hpp"

namespace fs = std::filesystem;

namespace vira::tools {
    struct CraterLabel {
        size_t id_num;
        Pixel bmin;
        Pixel bmax;
    };

    template <IsFloat TFloat>
    struct RobbinsEntry {
        RobbinsEntry() = default;
        RobbinsEntry(const std::vector<std::string>& row, size_t idx, vira::geometry::Ellipsoid<TFloat>& ellipsoid)
        {
            this->id = row[0]; // 0
            this->id_num = idx;

            this->LAT_CIRC_IMG = std::stof(row[1]); // 1
            this->LON_CIRC_IMG = std::stof(row[2]); // 2
            this->DIAM_CIRC_IMG = 1000 * std::stof(row[5]) / 2.f; // 5
            this->XYZ_CIRC = ellipsoid.computePoint(LAT_CIRC_IMG, LON_CIRC_IMG, true);

            if (!row[3].empty()) {
                this->LAT_ELLI_IMG = std::stof(row[3]); // 3
                this->LON_ELLI_IMG = std::stof(row[4]); // 4
                this->DIAM_ELLI_MAJOR_IMG = 1000 * std::stof(row[7]) / 2.f; // 7
                this->DIAM_ELLI_MINOR_IMG = 1000 * std::stof(row[8]) / 2.f; // 8
                this->DIAM_ELLI_ANGLE_IMG = std::stof(row[11]); // 11
            }
            else {
                this->LAT_ELLI_IMG = this->LAT_CIRC_IMG;
                this->LON_ELLI_IMG = this->LON_CIRC_IMG;
                this->DIAM_ELLI_MAJOR_IMG = this->DIAM_CIRC_IMG;
                this->DIAM_ELLI_MINOR_IMG = this->DIAM_CIRC_IMG;
                this->DIAM_ELLI_ANGLE_IMG = 0;
            }

            this->XYZ_ELLI = ellipsoid.computePoint(LAT_ELLI_IMG, LON_ELLI_IMG, true);
        }

        std::string id = ""; // 0
        size_t id_num = std::numeric_limits<size_t>::max();

        TFloat LAT_CIRC_IMG; // 1
        TFloat LON_CIRC_IMG; // 2
        TFloat DIAM_CIRC_IMG; // 5
        vira::vec3<TFloat> XYZ_CIRC;

        TFloat LAT_ELLI_IMG; // 3
        TFloat LON_ELLI_IMG; // 4
        TFloat DIAM_ELLI_MAJOR_IMG; // 7
        TFloat DIAM_ELLI_MINOR_IMG; // 8
        TFloat DIAM_ELLI_ANGLE_IMG; // 11
        vira::vec3<TFloat> XYZ_ELLI;
    };

    template <IsFloat TFloat>
    std::vector<RobbinsEntry<TFloat>> readRobbins(fs::path robbins_cat, vira::geometry::Ellipsoid<TFloat>& lunarEllipsoid)
    {
        lunarEllipsoid.initialize();
        std::ifstream file(robbins_cat);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open " + robbins_cat.string());
        }

        std::string line;

        // Count all lines:
        size_t rowCount = 0;
        while (std::getline(file, line)) {
            rowCount++;
        }
        file.clear();
        file.seekg(0, std::ios::beg);

        // Skip header:
        rowCount = rowCount - 1;
        std::getline(file, line);

        // Loop over all rows:
        std::vector<RobbinsEntry<TFloat>> robbins(rowCount);
        size_t idx = 0;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string value;

            std::vector<std::string> row;
            while (std::getline(ss, value, ',')) {
                row.push_back(value);
            }

            RobbinsEntry<TFloat> entry(row, idx, lunarEllipsoid);

            robbins[idx] = entry;
            idx++;
        }

        lunarEllipsoid.free();
        return robbins;
    };
};

#endif