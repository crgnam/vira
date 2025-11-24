#ifndef VIRA_DEMS_SET_PROJ_DATA_HPP
#define VIRA_DEMS_SET_PROJ_DATA_HPP

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace vira::dems {
    inline void setProjDataSearchPaths(std::vector<fs::path> customSearchPaths = {});
};

#include "implementation/dems/set_proj_data.ipp"

#endif