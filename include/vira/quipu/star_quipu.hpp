#ifndef VIRA_QUIPU_STAR_QUIPU_HPP
#define VIRA_QUIPU_STAR_QUIPU_HPP

#include <filesystem>

namespace fs = std::filesystem;

namespace vira::unresolved {
    // Forward Declare:
    class StarCatalogue;
};

namespace vira::quipu {
    struct StarQuipuWriterOptions {
        
    };

    struct StarQuipuReaderOptions {

    };

    class StarQuipu {
    public:
        // TODO Do some fancy dynamic loading of subsets of stars:
        StarQuipu() = default;
        //StarQuipu(const fs::path& filepath, StarQuipuReaderOptions options = StarQuipuReaderOptions{});

        inline static vira::unresolved::StarCatalogue read(const fs::path& filepath);
        inline static void write(fs::path file, const vira::unresolved::StarCatalogue& stars);
    };
};

#include "implementation/quipu/star_quipu.ipp"

#endif