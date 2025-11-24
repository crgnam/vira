#include <filesystem>
#include <vector>
#include <iostream>

#include "gdal.h"
#include "cpl_string.h"

#include "vira/utils/print_utils.hpp"

namespace fs = std::filesystem;

namespace vira::dems {
    /// @brief Global flag to track whether PROJ data paths have been configured
    static bool PROJ_PATH_SET = false;

    /**
     * @brief Configures PROJ library data search paths for coordinate transformations
     * @param customSearchPaths Vector of filesystem paths to add to PROJ library search
     * 
     * @details This function sets up the PROJ_LIB environment variable that tells the
     *          PROJ library where to find its coordinate system definition files.
     *          The function performs the following operations:
     *          
     *          1. Checks if paths have already been set using the PROJ_PATH_SET flag
     *          2. Starts with the default PROJ directory (MY_PROJ_DIR)
     *          3. Appends custom search paths using platform-appropriate delimiters
     *          4. Sets the PROJ_LIB configuration option via GDAL's CPL interface
     *          5. Marks the configuration as complete to prevent re-initialization
     *          
     *          The search paths are concatenated using:
     *          - Semicolon (;) on Windows platforms
     *          - Colon (:) on Unix-like platforms (Linux, macOS)
     * 
     * @note This function can only be called once per application run due to the
     *       PROJ_PATH_SET guard. Subsequent calls are ignored to prevent conflicts.
     *
     * @note This function is automatically called with the default search arguments by
     *       all functions/classes that use PROJ.  If you need to include an additional
     *       search path, then you must manually call this function before using any of
     *       the Vira API since it can only be called once per application
     * 
     * @note Debug output is conditionally printed based on vira::getPrintStatus().
     *       When enabled, the function displays all configured search paths in yellow text.
     * 
     * @warning The MY_PROJ_DIR macro must be defined at compile time, typically pointing
     *          to the installation directory of PROJ data files.
     * 
     * @see CPLSetConfigOption() for the underlying GDAL configuration mechanism
     * @see PROJ library documentation for details on coordinate system data files
     */
    void setProjDataSearchPaths(std::vector<fs::path> customSearchPaths)
    {
        if (!PROJ_PATH_SET) {
            if (vira::getPrintStatus()) {
                std::cout << vira::print::printYellow("Setting default PROJ_DATA paths to: \n");
                std::cout << "    " << vira::print::printYellow(MY_PROJ_DIR) << "\n";
            }

            std::string searchPaths = MY_PROJ_DIR;

            std::string delim;
#ifdef _WIN32
            delim = ";";
#else
            delim = ":";
#endif

            for (size_t i = 0; i < customSearchPaths.size(); ++i) {
                searchPaths += delim + customSearchPaths[i].string();
                if (vira::getPrintStatus()) {
                    std::cout << "    " << vira::print::printYellow(customSearchPaths[i].string()) << "\n";
                }
            }

            CPLSetConfigOption("PROJ_LIB", searchPaths.c_str());

            PROJ_PATH_SET = true;
            std::cout << "\n";
        }
        
    };
};