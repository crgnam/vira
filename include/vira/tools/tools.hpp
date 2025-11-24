#ifndef VIRA_TOOLS_TOOLS_HPP
#define VIRA_TOOLS_TOOLS_HPP

#include <string>
#include <vector>

#include "vira/constraints.hpp"

namespace vira::tools {
    void pds2tif(const std::string& inputFile, const std::string& outputFile, const std::vector<std::string>& options);
};

#endif